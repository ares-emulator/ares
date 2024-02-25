use crate::{FilterMode, WrapMode};
use windows::Win32::Graphics::Direct3D12;

impl From<WrapMode> for Direct3D12::D3D12_TEXTURE_ADDRESS_MODE {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => Direct3D12::D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            WrapMode::ClampToEdge => Direct3D12::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            WrapMode::Repeat => Direct3D12::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            WrapMode::MirroredRepeat => Direct3D12::D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
        }
    }
}

impl From<FilterMode> for Direct3D12::D3D12_FILTER {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => Direct3D12::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            _ => Direct3D12::D3D12_FILTER_MIN_MAG_MIP_POINT,
        }
    }
}
