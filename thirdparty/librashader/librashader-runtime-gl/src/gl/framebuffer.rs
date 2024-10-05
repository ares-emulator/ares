use crate::error::{FilterChainError, Result};
use crate::framebuffer::GLImage;
use crate::gl::FramebufferInterface;
use crate::texture::InputTexture;
use glow::HasContext;
use librashader_common::{FilterMode, GetSize, ImageFormat, Size, WrapMode};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::ScaleFramebuffer;
use std::sync::Arc;

/// A handle to an OpenGL FBO and its backing texture with format and size information.
///
/// Generally for use as render targets.
#[derive(Debug)]
pub struct GLFramebuffer {
    pub(crate) image: Option<glow::Texture>,
    pub(crate) fbo: glow::Framebuffer,
    pub(crate) size: Size<u32>,
    pub(crate) format: u32,
    pub(crate) max_levels: u32,
    pub(crate) mip_levels: u32,
    pub(crate) is_extern_image: bool,
    pub(crate) ctx: Arc<glow::Context>,
}

impl GLFramebuffer {
    /// Create a framebuffer from an already initialized texture and framebuffer.
    ///
    /// The framebuffer will not be deleted when this struct is dropped.
    pub fn new_from_raw(
        ctx: Arc<glow::Context>,
        texture: Option<glow::Texture>,
        fbo: glow::Framebuffer,
        format: u32,
        size: Size<u32>,
        miplevels: u32,
    ) -> GLFramebuffer {
        GLFramebuffer {
            image: texture,
            size,
            format,
            max_levels: miplevels,
            mip_levels: miplevels,
            fbo,
            is_extern_image: true,
            ctx,
        }
    }

    pub(crate) fn clear<T: FramebufferInterface, const REBIND: bool>(&self) {
        T::clear::<REBIND>(self)
    }

    pub(crate) fn scale<T: FramebufferInterface>(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        mipmap: bool,
    ) -> Result<Size<u32>> {
        T::scale(
            self,
            scaling,
            format,
            viewport,
            source_size,
            original_size,
            mipmap,
        )
    }

    pub(crate) fn copy_from<T: FramebufferInterface>(&mut self, image: &GLImage) -> Result<()> {
        T::copy_from(self, image)
    }

    pub(crate) fn as_texture(&self, filter: FilterMode, wrap_mode: WrapMode) -> InputTexture {
        InputTexture {
            image: GLImage {
                handle: self.image,
                format: self.format,
                size: self.size,
            },
            filter,
            mip_filter: filter,
            wrap_mode,
        }
    }

    pub(crate) fn bind<T: FramebufferInterface>(&self) -> Result<()> {
        T::bind(self)
    }
}

/// A state-checked wrapper around a raw framebuffer, used exclusively for output images.
pub struct OutputFramebuffer {
    framebuffer: Option<GLFramebuffer>,
    ctx: Arc<glow::Context>,
}

impl OutputFramebuffer {
    pub fn new(ctx: &Arc<glow::Context>) -> Self {
        OutputFramebuffer {
            ctx: Arc::clone(ctx),
            framebuffer: None,
        }
    }

    /// Ensure that the renderbuffer is up to date.
    pub fn ensure<T: FramebufferInterface>(&mut self, image: &GLImage) -> Result<&GLFramebuffer> {
        let texture = image.handle;
        let size = image.size;
        let format = image.format;

        let Some(framebuffer) = self.framebuffer.as_mut() else {
            self.framebuffer = Some(T::new_raw(&self.ctx, texture, size, format, 1)?);
            return Ok(self.framebuffer.as_ref().unwrap());
        };

        assert!(
            framebuffer.is_extern_image,
            "Somehow an internal image got into the renderbuffer!"
        );

        if framebuffer.image == texture && framebuffer.size == size && framebuffer.format == format
        {
            // Problem Case #3 :cry:
            return Ok(self.framebuffer.as_ref().unwrap());
        };

        // Replace with a new framebuffer.
        let new = T::new_raw(&self.ctx, texture, size, format, 1)?;
        std::mem::swap(&mut self.framebuffer, &mut Some(new));
        Ok(self.framebuffer.as_ref().unwrap())
    }
}

impl Drop for GLFramebuffer {
    fn drop(&mut self) {
        unsafe {
            self.ctx.delete_framebuffer(self.fbo);

            if self.is_extern_image {
                return;
            }

            if let Some(image) = self.image {
                self.ctx.delete_texture(image);
            }
        }
    }
}
//
impl<T: FramebufferInterface> ScaleFramebuffer<T> for GLFramebuffer {
    type Error = FilterChainError;
    type Context = ();

    fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        should_mipmap: bool,
        _context: &Self::Context,
    ) -> Result<Size<u32>> {
        self.scale::<T>(
            scaling,
            format,
            viewport_size,
            source_size,
            original_size,
            should_mipmap,
        )
    }
}

impl GetSize<u32> for GLFramebuffer {
    type Error = std::convert::Infallible;

    fn size(&self) -> std::result::Result<Size<u32>, Self::Error> {
        Ok(self.size)
    }
}

impl GetSize<u32> for &GLFramebuffer {
    type Error = std::convert::Infallible;

    fn size(&self) -> std::result::Result<Size<u32>, Self::Error> {
        Ok(self.size)
    }
}
