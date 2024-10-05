use crate::error::Result;
use crate::framebuffer::OwnedImage;
use librashader_common::{FilterMode, WrapMode};
use windows::Win32::Graphics::Direct3D11::ID3D11ShaderResourceView;

#[derive(Debug, Clone)]
pub struct InputTexture {
    pub view: ID3D11ShaderResourceView,
    pub filter: FilterMode,
    pub wrap_mode: WrapMode,
}

impl InputTexture {
    pub(crate) fn from_framebuffer(
        fbo: &OwnedImage,
        wrap_mode: WrapMode,
        filter: FilterMode,
    ) -> Result<Self> {
        Ok(InputTexture {
            view: fbo.create_shader_resource_view()?,
            filter,
            wrap_mode,
        })
    }
}

impl AsRef<InputTexture> for InputTexture {
    fn as_ref(&self) -> &InputTexture {
        self
    }
}
