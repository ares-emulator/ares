use crate::texture::InputTexture;
use librashader_common::{FilterMode, GetSize, Size, WrapMode};

/// A handle to an OpenGL texture with format and size information.
///
/// Generally for use as shader resource inputs.
#[derive(Default, Debug, Copy, Clone)]
pub struct GLImage {
    /// A GLuint to the texture.
    pub handle: Option<glow::Texture>,
    /// The format of the texture.
    pub format: u32,
    /// The size of the texture.
    pub size: Size<u32>,
}

impl GLImage {
    pub(crate) fn as_texture(&self, filter: FilterMode, wrap_mode: WrapMode) -> InputTexture {
        InputTexture {
            image: *self,
            filter,
            mip_filter: filter,
            wrap_mode,
        }
    }
}

impl GetSize<u32> for GLImage {
    type Error = std::convert::Infallible;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        Ok(self.size)
    }
}

impl GetSize<u32> for &GLImage {
    type Error = std::convert::Infallible;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        Ok(self.size)
    }
}
