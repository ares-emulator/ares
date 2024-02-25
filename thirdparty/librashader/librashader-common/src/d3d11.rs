use crate::{FilterMode, WrapMode};
use windows::Win32::Graphics::Direct3D11;

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
