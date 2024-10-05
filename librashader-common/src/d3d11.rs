use crate::{FilterMode, GetSize, Size, WrapMode};
use windows::Win32::Foundation::E_NOINTERFACE;
use windows::Win32::Graphics::Direct3D11;
use windows::Win32::Graphics::Direct3D11::{ID3D11Texture2D, D3D11_RESOURCE_DIMENSION_TEXTURE2D};

impl From<WrapMode> for Direct3D11::D3D11_TEXTURE_ADDRESS_MODE {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => Direct3D11::D3D11_TEXTURE_ADDRESS_BORDER,
            WrapMode::ClampToEdge => Direct3D11::D3D11_TEXTURE_ADDRESS_CLAMP,
            WrapMode::Repeat => Direct3D11::D3D11_TEXTURE_ADDRESS_WRAP,
            WrapMode::MirroredRepeat => Direct3D11::D3D11_TEXTURE_ADDRESS_MIRROR,
        }
    }
}

impl From<FilterMode> for Direct3D11::D3D11_FILTER {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => Direct3D11::D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            FilterMode::Nearest => Direct3D11::D3D11_FILTER_MIN_MAG_MIP_POINT,
        }
    }
}

impl GetSize<u32> for &Direct3D11::ID3D11RenderTargetView {
    type Error = windows::core::Error;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        let parent: ID3D11Texture2D = unsafe {
            let resource = self.GetResource()?;
            if resource.GetType() != D3D11_RESOURCE_DIMENSION_TEXTURE2D {
                return Err(windows::core::Error::new(
                    E_NOINTERFACE,
                    "expected ID3D11Texture2D as the resource for the view.",
                ));
            }
            // SAFETY: We know tha the resource is an `ID3D11Texture2D`.
            // This downcast is safe because ID3D11Texture2D has ID3D11Resource in its
            // inheritance chain.
            //
            // This check + transmute is cheaper than doing `.cast` (i.e. `QueryInterface`).
            std::mem::transmute(resource)
        };

        let mut desc = Default::default();
        unsafe {
            parent.GetDesc(&mut desc);
        }

        Ok(Size {
            height: desc.Height,
            width: desc.Width,
        })
    }
}

impl GetSize<u32> for Direct3D11::ID3D11RenderTargetView {
    type Error = windows::core::Error;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        <&Self as GetSize<u32>>::size(&self)
    }
}

impl GetSize<u32> for &Direct3D11::ID3D11ShaderResourceView {
    type Error = windows::core::Error;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        let parent: ID3D11Texture2D = unsafe {
            let resource = self.GetResource()?;
            if resource.GetType() != D3D11_RESOURCE_DIMENSION_TEXTURE2D {
                return Err(windows::core::Error::new(
                    E_NOINTERFACE,
                    "expected ID3D11Texture2D as the resource for the view.",
                ));
            }
            // SAFETY: We know tha the resource is an `ID3D11Texture2D`.
            // This downcast is safe because ID3D11Texture2D has ID3D11Resource in its
            // inheritance chain.
            //
            // This check + transmute is cheaper than doing `.cast` (i.e. `QueryInterface`).
            std::mem::transmute(resource)
        };
        let mut desc = Default::default();
        unsafe {
            parent.GetDesc(&mut desc);
        }

        Ok(Size {
            height: desc.Height,
            width: desc.Width,
        })
    }
}

impl GetSize<u32> for Direct3D11::ID3D11ShaderResourceView {
    type Error = windows::core::Error;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        <&Self as GetSize<u32>>::size(&self)
    }
}
