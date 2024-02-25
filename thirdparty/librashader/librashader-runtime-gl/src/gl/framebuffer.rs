use crate::error::{FilterChainError, Result};
use crate::framebuffer::GLImage;
use crate::gl::FramebufferInterface;
use crate::texture::InputTexture;
use gl::types::{GLenum, GLuint};
use librashader_common::{FilterMode, ImageFormat, Size, WrapMode};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::ScaleFramebuffer;

/// A handle to an OpenGL FBO and its backing texture with format and size information.
///
/// Generally for use as render targets.
#[derive(Debug)]
pub struct GLFramebuffer {
    pub(crate) image: GLuint,
    pub(crate) fbo: GLuint,
    pub(crate) size: Size<u32>,
    pub(crate) format: GLenum,
    pub(crate) max_levels: u32,
    pub(crate) mip_levels: u32,
    pub(crate) is_raw: bool,
}

impl GLFramebuffer {
    /// Create a framebuffer from an already initialized texture and framebuffer.
    ///
    /// The framebuffer will not be deleted when this struct is dropped.
    pub fn new_from_raw(
        texture: GLuint,
        fbo: GLuint,
        format: GLenum,
        size: Size<u32>,
        miplevels: u32,
    ) -> GLFramebuffer {
        GLFramebuffer {
            image: texture,
            size,
            format,
            max_levels: miplevels,
            mip_levels: miplevels,
            fbo: fbo,
            is_raw: true,
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
}

impl Drop for GLFramebuffer {
    fn drop(&mut self) {
        if self.is_raw {
            return;
        }

        unsafe {
            if self.fbo != 0 {
                gl::DeleteFramebuffers(1, &self.fbo);
            }
            if self.image != 0 {
                gl::DeleteTextures(1, &self.image);
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
