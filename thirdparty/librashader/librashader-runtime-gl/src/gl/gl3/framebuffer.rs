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
pub struct Gl3Framebuffer;

impl FramebufferInterface for Gl3Framebuffer {
    fn new(ctx: &Arc<glow::Context>, max_levels: u32) -> error::Result<GLFramebuffer> {
        unsafe {
            let framebuffer = ctx
                .create_framebuffer()
                .map_err(FilterChainError::GlError)?;
            ctx.bind_framebuffer(glow::FRAMEBUFFER, Some(framebuffer));
            ctx.bind_framebuffer(glow::FRAMEBUFFER, None);

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
                ctx: Arc::clone(&ctx),
            })
        }
    }

    fn new_raw(
        ctx: &Arc<glow::Context>,
        image: Option<glow::Texture>,
        mut size: Size<u32>,
        format: u32,
        miplevels: u32,
    ) -> Result<GLFramebuffer> {
        let framebuffer = unsafe {
            ctx.create_framebuffer()
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
            ctx.bind_framebuffer(glow::FRAMEBUFFER, Some(framebuffer));

            ctx.framebuffer_texture_2d(
                glow::FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                image,
                0,
            );

            ctx.check_framebuffer_status(glow::FRAMEBUFFER)
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
            if REBIND {
                fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, Some(fb.fbo));
            }
            fb.ctx.color_mask(true, true, true, true);
            fb.ctx.clear_color(0.0, 0.0, 0.0, 0.0);
            fb.ctx.clear(glow::COLOR_BUFFER_BIT);
            if REBIND {
                fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, None);
            }
        }
    }
    fn copy_from(fb: &mut GLFramebuffer, image: &GLImage) -> Result<()> {
        // todo: may want to use a shader and draw a quad to be faster.
        if image.size != fb.size || image.format != fb.format {
            Self::init(fb, image.size, image.format)?;
        }

        unsafe {
            fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, Some(fb.fbo));

            fb.ctx.framebuffer_texture_2d(
                glow::READ_FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                image.handle,
                0,
            );
            fb.ctx.framebuffer_texture_2d(
                glow::DRAW_FRAMEBUFFER,
                glow::COLOR_ATTACHMENT1,
                glow::TEXTURE_2D,
                fb.image,
                0,
            );

            fb.ctx.read_buffer(glow::COLOR_ATTACHMENT0);
            fb.ctx.draw_buffer(glow::COLOR_ATTACHMENT1);

            fb.ctx.blit_framebuffer(
                0,
                0,
                fb.size.width as i32,
                fb.size.height as i32,
                0,
                0,
                fb.size.width as i32,
                fb.size.height as i32,
                glow::COLOR_BUFFER_BIT,
                glow::NEAREST,
            );

            // cleanup after ourselves.
            fb.ctx.framebuffer_texture_2d(
                glow::READ_FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                None,
                0,
            );
            fb.ctx.framebuffer_texture_2d(
                glow::DRAW_FRAMEBUFFER,
                glow::COLOR_ATTACHMENT1,
                glow::TEXTURE_2D,
                None,
                0,
            );

            // set this back to color_attachment 0
            fb.ctx.framebuffer_texture_2d(
                glow::FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                fb.image,
                0,
            );

            fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, None);
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
            fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, Some(fb.fbo));

            // reset the framebuffer image
            if let Some(image) = fb.image {
                fb.ctx.framebuffer_texture_2d(
                    glow::FRAMEBUFFER,
                    glow::COLOR_ATTACHMENT0,
                    glow::TEXTURE_2D,
                    None,
                    0,
                );

                fb.ctx.delete_texture(image);
            }

            fb.image = Some(fb.ctx.create_texture().map_err(FilterChainError::GlError)?);
            fb.ctx.bind_texture(glow::TEXTURE_2D, fb.image);

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

            fb.ctx.tex_storage_2d(
                glow::TEXTURE_2D,
                fb.mip_levels as i32,
                fb.format,
                size.width as i32,
                size.height as i32,
            );

            fb.ctx.framebuffer_texture_2d(
                glow::FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                fb.image,
                0,
            );

            let status = fb.ctx.check_framebuffer_status(glow::FRAMEBUFFER);
            if status != glow::FRAMEBUFFER_COMPLETE {
                match status {
                    glow::FRAMEBUFFER_UNSUPPORTED => {
                        fb.ctx.framebuffer_texture_2d(
                            glow::FRAMEBUFFER,
                            glow::COLOR_ATTACHMENT0,
                            glow::TEXTURE_2D,
                            None,
                            0,
                        );

                        if let Some(image) = fb.image {
                            fb.ctx.delete_texture(image);
                        }

                        fb.image =
                            Some(fb.ctx.create_texture().map_err(FilterChainError::GlError)?);
                        fb.ctx.bind_texture(glow::TEXTURE_2D, fb.image);

                        fb.mip_levels = size.calculate_miplevels();
                        if fb.mip_levels > fb.max_levels {
                            fb.mip_levels = fb.max_levels;
                        }
                        if fb.mip_levels == 0 {
                            fb.mip_levels = 1;
                        }

                        fb.ctx.tex_storage_2d(
                            glow::TEXTURE_2D,
                            fb.mip_levels as i32,
                            ImageFormat::R8G8B8A8Unorm.into(),
                            size.width as i32,
                            size.height as i32,
                        );

                        fb.ctx.framebuffer_texture_2d(
                            glow::FRAMEBUFFER,
                            glow::COLOR_ATTACHMENT0,
                            glow::TEXTURE_2D,
                            fb.image,
                            0,
                        );

                        // fb.init =
                        //     glow::CheckFramebufferStatus(glow::FRAMEBUFFER) == glow::FRAMEBUFFER_COMPLETE;
                    }
                    _ => return Err(FilterChainError::FramebufferInit(status)),
                }
            }

            fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, None);
            fb.ctx.bind_texture(glow::TEXTURE_2D, None);
        }

        Ok(())
    }

    fn bind(fb: &GLFramebuffer) -> Result<()> {
        unsafe {
            fb.ctx.bind_framebuffer(glow::FRAMEBUFFER, Some(fb.fbo));
            fb.ctx.framebuffer_texture_2d(
                glow::FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                fb.image,
                0,
            );
            let status = fb.ctx.check_framebuffer_status(glow::FRAMEBUFFER);
            if status != glow::FRAMEBUFFER_COMPLETE {
                return Err(FilterChainError::FramebufferInit(status));
            }
        }

        Ok(())
    }
}
