use crate::error;
use crate::error::{FilterChainError, Result};
use crate::framebuffer::GLImage;
use crate::gl::framebuffer::GLFramebuffer;
use crate::gl::FramebufferInterface;
use glow::HasContext;
use librashader_common::{ImageFormat, Size};
use librashader_runtime::scaling::MipmapSize;
use std::sync::Arc;

#[derive(Debug)]
pub struct Gl46Framebuffer;

impl FramebufferInterface for Gl46Framebuffer {
    fn new(context: &Arc<glow::Context>, max_levels: u32) -> error::Result<GLFramebuffer> {
        let framebuffer = unsafe {
            context
                .create_named_framebuffer()
                .map_err(FilterChainError::GlError)?
        };

        Ok(GLFramebuffer {
            image: None,
            size: Size {
                width: 1,
                height: 1,
            },
            format: 0,
            max_levels,
            mip_levels: 0,
            fbo: framebuffer,
            is_extern_image: false,
            ctx: Arc::clone(&context),
        })
    }

    fn new_raw(
        ctx: &Arc<glow::Context>,
        image: Option<glow::Texture>,
        mut size: Size<u32>,
        format: u32,
        miplevels: u32,
    ) -> Result<GLFramebuffer> {
        let framebuffer = unsafe {
            ctx.create_named_framebuffer()
                .map_err(FilterChainError::GlError)?
        };

        if size.width == 0 {
            size.width = 1;
        }
        if size.height == 0 {
            size.height = 1;
        }

        if size.width > librashader_runtime::scaling::MAX_TEXEL_SIZE as u32 {
            size.width = librashader_runtime::scaling::MAX_TEXEL_SIZE as u32 - 1;
        }

        if size.height > librashader_runtime::scaling::MAX_TEXEL_SIZE as u32 {
            size.height = librashader_runtime::scaling::MAX_TEXEL_SIZE as u32 - 1;
        }

        let status = unsafe {
            ctx.named_framebuffer_texture(Some(framebuffer), glow::COLOR_ATTACHMENT0, image, 0);

            ctx.check_named_framebuffer_status(Some(framebuffer), glow::FRAMEBUFFER)
        };

        if status != glow::FRAMEBUFFER_COMPLETE {
            return Err(FilterChainError::FramebufferInit(status));
        }

        Ok(GLFramebuffer {
            image,
            size,
            format,
            max_levels: miplevels,
            mip_levels: miplevels,
            fbo: framebuffer,
            is_extern_image: true,
            ctx: Arc::clone(&ctx),
        })
    }

    fn clear<const REBIND: bool>(fb: &GLFramebuffer) {
        unsafe {
            fb.ctx.clear_named_framebuffer_f32_slice(
                Some(fb.fbo),
                glow::COLOR,
                0,
                &[0.0f32, 0.0, 0.0, 0.0],
            );
        }
    }
    fn copy_from(fb: &mut GLFramebuffer, image: &GLImage) -> Result<()> {
        // todo: confirm this behaviour for unbound image.
        if image.handle == None {
            return Ok(());
        }

        // todo: may want to use a shader and draw a quad to be faster.
        if image.size != fb.size || image.format != fb.format {
            Self::init(fb, image.size, image.format)?;
        }

        unsafe {
            // glow::NamedFramebufferDrawBuffer(fb.handle, glow::COLOR_ATTACHMENT1);
            fb.ctx
                .named_framebuffer_read_buffer(Some(fb.fbo), glow::COLOR_ATTACHMENT0);
            fb.ctx
                .named_framebuffer_draw_buffer(Some(fb.fbo), glow::COLOR_ATTACHMENT1);

            fb.ctx.named_framebuffer_texture(
                Some(fb.fbo),
                glow::COLOR_ATTACHMENT0,
                image.handle,
                0,
            );
            fb.ctx
                .named_framebuffer_texture(Some(fb.fbo), glow::COLOR_ATTACHMENT1, fb.image, 0);

            fb.ctx.blit_named_framebuffer(
                Some(fb.fbo),
                Some(fb.fbo),
                0,
                0,
                image.size.width as i32,
                image.size.height as i32,
                0,
                0,
                fb.size.width as i32,
                fb.size.height as i32,
                glow::COLOR_BUFFER_BIT,
                glow::NEAREST,
            );
        }

        Ok(())
    }
    fn init(fb: &mut GLFramebuffer, mut size: Size<u32>, format: impl Into<u32>) -> Result<()> {
        if fb.is_extern_image {
            return Ok(());
        }
        fb.format = format.into();
        fb.size = size;

        unsafe {
            // reset the framebuffer image
            if let Some(image) = fb.image {
                fb.ctx
                    .named_framebuffer_texture(Some(fb.fbo), glow::COLOR_ATTACHMENT0, None, 0);
                fb.ctx.delete_texture(image);
            }

            let image = fb
                .ctx
                .create_named_texture(glow::TEXTURE_2D)
                .map_err(FilterChainError::GlError)?;

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

            fb.ctx.texture_storage_2d(
                image,
                fb.mip_levels as i32,
                fb.format,
                size.width as i32,
                size.height as i32,
            );

            fb.image = Some(image);

            fb.ctx
                .named_framebuffer_texture(Some(fb.fbo), glow::COLOR_ATTACHMENT0, fb.image, 0);

            let status = fb
                .ctx
                .check_named_framebuffer_status(Some(fb.fbo), glow::FRAMEBUFFER);
            if status != glow::FRAMEBUFFER_COMPLETE {
                match status {
                    glow::FRAMEBUFFER_UNSUPPORTED => {
                        fb.ctx.named_framebuffer_texture(
                            Some(fb.fbo),
                            glow::COLOR_ATTACHMENT0,
                            None,
                            0,
                        );
                        fb.ctx.delete_texture(image);

                        let image = fb
                            .ctx
                            .create_named_texture(glow::TEXTURE_2D)
                            .map_err(FilterChainError::GlError)?;

                        fb.mip_levels = size.calculate_miplevels();
                        if fb.mip_levels > fb.max_levels {
                            fb.mip_levels = fb.max_levels;
                        }
                        if fb.mip_levels == 0 {
                            fb.mip_levels = 1;
                        }

                        fb.ctx.texture_storage_2d(
                            image,
                            fb.mip_levels as i32,
                            ImageFormat::R8G8B8A8Unorm.into(),
                            size.width as i32,
                            size.height as i32,
                        );

                        fb.image = Some(image);

                        fb.ctx.named_framebuffer_texture(
                            Some(fb.fbo),
                            glow::COLOR_ATTACHMENT0,
                            fb.image,
                            0,
                        );
                    }
                    _ => return Err(FilterChainError::FramebufferInit(status)),
                }
            }
        }
        Ok(())
    }

    fn bind(fb: &GLFramebuffer) -> Result<()> {
        unsafe {
            fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, Some(fb.fbo));
            let status = fb.ctx.check_framebuffer_status(glow::FRAMEBUFFER);
            if status != glow::FRAMEBUFFER_COMPLETE {
                return Err(FilterChainError::FramebufferInit(status));
            }
        }

        Ok(())
    }
}
