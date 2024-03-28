use crate::error::{FilterChainError, Result};
use crate::framebuffer::GLImage;
use crate::gl::framebuffer::GLFramebuffer;
use crate::gl::FramebufferInterface;
use gl::types::{GLenum, GLint, GLsizei};
use librashader_common::{ImageFormat, Size};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::{MipmapSize, ViewportSize};

#[derive(Debug)]
pub struct Gl46Framebuffer;

impl FramebufferInterface for Gl46Framebuffer {
    fn new(max_levels: u32) -> GLFramebuffer {
        let mut framebuffer = 0;
        unsafe {
            gl::CreateFramebuffers(1, &mut framebuffer);
        }

        GLFramebuffer {
            image: 0,
            size: Size {
                width: 1,
                height: 1,
            },
            format: 0,
            max_levels,
            mip_levels: 0,
            fbo: framebuffer,
            is_raw: false,
        }
    }

    fn scale(
        fb: &mut GLFramebuffer,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        mipmap: bool,
    ) -> Result<Size<u32>> {
        if fb.is_raw {
            return Ok(fb.size);
        }

        let size = source_size.scale_viewport(scaling, *viewport_size, *original_size);

        if fb.size != size || (mipmap && fb.max_levels == 1) || (!mipmap && fb.max_levels != 1) {
            fb.size = size;

            if mipmap {
                fb.max_levels = u32::MAX;
            } else {
                fb.max_levels = 1
            }

            Self::init(
                fb,
                size,
                if format == ImageFormat::Unknown {
                    ImageFormat::R8G8B8A8Unorm
                } else {
                    format
                },
            )?;
        }
        Ok(size)
    }
    fn clear<const REBIND: bool>(fb: &GLFramebuffer) {
        unsafe {
            gl::ClearNamedFramebufferfv(
                fb.fbo,
                gl::COLOR,
                0,
                [0.0f32, 0.0, 0.0, 0.0].as_ptr().cast(),
            );
        }
    }
    fn copy_from(fb: &mut GLFramebuffer, image: &GLImage) -> Result<()> {
        // todo: confirm this behaviour for unbound image.
        if image.size != fb.size || image.format != fb.format {
            Self::init(fb, image.size, image.format)?;
        }

        if image.handle == 0 {
            return Ok(());
        }

        unsafe {
            // gl::NamedFramebufferDrawBuffer(fb.handle, gl::COLOR_ATTACHMENT1);
            gl::NamedFramebufferReadBuffer(fb.fbo, gl::COLOR_ATTACHMENT0);
            gl::NamedFramebufferDrawBuffer(fb.fbo, gl::COLOR_ATTACHMENT1);

            gl::NamedFramebufferTexture(fb.fbo, gl::COLOR_ATTACHMENT0, image.handle, 0);
            gl::NamedFramebufferTexture(fb.fbo, gl::COLOR_ATTACHMENT1, fb.image, 0);

            gl::BlitNamedFramebuffer(
                fb.fbo,
                fb.fbo,
                0,
                0,
                image.size.width as GLint,
                image.size.height as GLint,
                0,
                0,
                fb.size.width as GLint,
                fb.size.height as GLint,
                gl::COLOR_BUFFER_BIT,
                gl::NEAREST,
            );
        }

        Ok(())
    }
    fn init(fb: &mut GLFramebuffer, mut size: Size<u32>, format: impl Into<GLenum>) -> Result<()> {
        if fb.is_raw {
            return Ok(());
        }
        fb.format = format.into();
        fb.size = size;

        unsafe {
            // reset the framebuffer image
            if fb.image != 0 {
                gl::NamedFramebufferTexture(fb.fbo, gl::COLOR_ATTACHMENT0, 0, 0);
                gl::DeleteTextures(1, &fb.image);
            }

            gl::CreateTextures(gl::TEXTURE_2D, 1, &mut fb.image);

            if size.width == 0 {
                size.width = 1;
            }
            if size.height == 0 {
                size.height = 1;
            }

            fb.mip_levels = size.calculate_miplevels();
            if fb.mip_levels > fb.max_levels {
                fb.mip_levels = fb.max_levels;
            }
            if fb.mip_levels == 0 {
                fb.mip_levels = 1;
            }

            gl::TextureStorage2D(
                fb.image,
                fb.mip_levels as GLsizei,
                fb.format,
                size.width as GLsizei,
                size.height as GLsizei,
            );

            gl::NamedFramebufferTexture(fb.fbo, gl::COLOR_ATTACHMENT0, fb.image, 0);

            let status = gl::CheckFramebufferStatus(gl::FRAMEBUFFER);
            if status != gl::FRAMEBUFFER_COMPLETE {
                match status {
                    gl::FRAMEBUFFER_UNSUPPORTED => {
                        gl::NamedFramebufferTexture(fb.fbo, gl::COLOR_ATTACHMENT0, 0, 0);
                        gl::DeleteTextures(1, &fb.image);
                        gl::CreateTextures(gl::TEXTURE_2D, 1, &mut fb.image);

                        fb.mip_levels = size.calculate_miplevels();
                        if fb.mip_levels > fb.max_levels {
                            fb.mip_levels = fb.max_levels;
                        }
                        if fb.mip_levels == 0 {
                            fb.mip_levels = 1;
                        }

                        gl::TextureStorage2D(
                            fb.image,
                            fb.mip_levels as GLsizei,
                            ImageFormat::R8G8B8A8Unorm.into(),
                            size.width as GLsizei,
                            size.height as GLsizei,
                        );
                        gl::NamedFramebufferTexture(fb.fbo, gl::COLOR_ATTACHMENT0, fb.image, 0);
                        // fb.init =
                        //     gl::CheckFramebufferStatus(gl::FRAMEBUFFER) == gl::FRAMEBUFFER_COMPLETE;
                    }
                    _ => return Err(FilterChainError::FramebufferInit(status)),
                }
            }
        }
        Ok(())
    }
}