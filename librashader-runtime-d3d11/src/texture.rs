use crate::error::Result;
use crate::framebuffer::OwnedImage;
use librashader_common::{FilterMode, Size, WrapMode};
use windows::Win32::Graphics::Direct3D11::{ID3D11RenderTargetView, ID3D11ShaderResourceView};

/// An image view for use as a shader resource.
///
/// Contains an `ID3D11ShaderResourceView`, and a size.
#[derive(Debug, Clone)]
pub struct D3D11InputView {
    /// A handle to the shader resource view.
    pub handle: ID3D11ShaderResourceView,
    /// The size of the image.
    pub size: Size<u32>,
}

/// An image view for use as a render target.
///
/// Contains an `ID3D11RenderTargetView`, and a size.
#[derive(Debug, Clone)]
pub struct D3D11OutputView {
    /// A handle to the render target view.
    pub handle: ID3D11RenderTargetView,
    /// The size of the image.
    pub size: Size<u32>,
}

#[derive(Debug, Clone)]
pub struct InputTexture {
    pub view: D3D11InputView,
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
            view: D3D11InputView {
                handle: fbo.create_shader_resource_view()?,
                size: fbo.size,
            },
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
