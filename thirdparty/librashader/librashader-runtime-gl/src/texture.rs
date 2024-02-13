use crate::framebuffer::GLImage;

use librashader_common::{FilterMode, WrapMode};

#[derive(Default, Debug, Copy, Clone)]
pub(crate) struct InputTexture {
    pub image: GLImage,
    pub filter: FilterMode,
    pub mip_filter: FilterMode,
    pub wrap_mode: WrapMode,
}

/// An OpenGL texture bound as a shader resource.
impl InputTexture {
    pub fn is_bound(&self) -> bool {
        self.image.handle != 0
    }

    /// Returns a reference to itself if the texture is bound.
    pub fn bound(&self) -> Option<&Self> {
        if self.is_bound() {
            Some(self)
        } else {
            None
        }
    }
}

impl AsRef<InputTexture> for InputTexture {
    fn as_ref(&self) -> &InputTexture {
        self
    }
}
