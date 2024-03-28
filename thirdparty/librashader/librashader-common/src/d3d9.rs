use crate::{FilterMode, ImageFormat, WrapMode};
use windows::Win32::Graphics::Direct3D9;
//
impl From<ImageFormat> for Direct3D9::D3DFORMAT {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => Direct3D9::D3DFMT_UNKNOWN,
            ImageFormat::R8Unorm => Direct3D9::D3DFMT_R8G8B8,
            ImageFormat::R8Uint => Direct3D9::D3DFMT_R8G8B8,
            ImageFormat::R8Sint => Direct3D9::D3DFMT_R8G8B8,
            ImageFormat::R8G8B8A8Unorm => Direct3D9::D3DFMT_A8R8G8B8,
            ImageFormat::R8G8B8A8Uint => Direct3D9::D3DFMT_A8R8G8B8,
            ImageFormat::R8G8B8A8Sint => Direct3D9::D3DFMT_A8R8G8B8,
            ImageFormat::R8G8B8A8Srgb => Direct3D9::D3DFMT_A8R8G8B8,
            ImageFormat::A2B10G10R10UnormPack32 => Direct3D9::D3DFMT_A2B10G10R10,
            ImageFormat::A2B10G10R10UintPack32 => Direct3D9::D3DFMT_A2B10G10R10,
            ImageFormat::R16Sfloat => Direct3D9::D3DFMT_R16F,
            ImageFormat::R16G16Uint => Direct3D9::D3DFMT_G16R16,
            ImageFormat::R16G16Sint => Direct3D9::D3DFMT_G16R16,
            ImageFormat::R16G16Sfloat => Direct3D9::D3DFMT_G16R16F,
            ImageFormat::R16G16B16A16Uint => Direct3D9::D3DFMT_A16B16G16R16,
            ImageFormat::R16G16B16A16Sint => Direct3D9::D3DFMT_A16B16G16R16,
            ImageFormat::R16G16B16A16Sfloat => Direct3D9::D3DFMT_A16B16G16R16F,
            ImageFormat::R32Sfloat => Direct3D9::D3DFMT_R32F,
            _ => Direct3D9::D3DFMT_UNKNOWN,
        }
    }
}
//
impl From<Direct3D9::D3DFORMAT> for ImageFormat {
    fn from(format: Direct3D9::D3DFORMAT) -> Self {
        match format {
            Direct3D9::D3DFMT_R8G8B8 => ImageFormat::R8Unorm,
            Direct3D9::D3DFMT_A8R8G8B8 => ImageFormat::R8G8B8A8Unorm,
            Direct3D9::D3DFMT_A2B10G10R10 => ImageFormat::A2B10G10R10UnormPack32,
            Direct3D9::D3DFMT_R16F => ImageFormat::R16Sfloat,
            Direct3D9::D3DFMT_G16R16 => ImageFormat::R16G16Uint,
            Direct3D9::D3DFMT_G16R16F => ImageFormat::R16G16Sfloat,
            Direct3D9::D3DFMT_A16B16G16R16 => ImageFormat::R16G16B16A16Uint,
            Direct3D9::D3DFMT_A16B16G16R16F => ImageFormat::R16G16B16A16Sfloat,
            Direct3D9::D3DFMT_R32F => ImageFormat::R32Sfloat,
            _ => ImageFormat::Unknown,
        }
    }
}

impl From<WrapMode> for Direct3D9::D3DTEXTUREADDRESS {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => Direct3D9::D3DTADDRESS_BORDER,
            WrapMode::ClampToEdge => Direct3D9::D3DTADDRESS_CLAMP,
            WrapMode::Repeat => Direct3D9::D3DTADDRESS_WRAP,
            WrapMode::MirroredRepeat => Direct3D9::D3DTADDRESS_MIRROR,
        }
    }
}

impl From<FilterMode> for Direct3D9::D3DTEXTUREFILTER {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => Direct3D9::D3DFILTER_LINEAR,
            FilterMode::Nearest => Direct3D9::D3DFILTER_NEAREST,
        }
    }
}

// impl FilterMode {
//     /// Get the mipmap filtering mode for the given combination.
//     pub fn d3d9_mip(&self, mip: FilterMode) -> Direct3D9::D3DTEXTUREFILTER {
//         match (self, mip) {
//             (FilterMode::Linear, FilterMode::Linear) => Direct3D9::D3DFILTER_MIPLINEAR,
//             (FilterMode::Linear, FilterMode::Nearest) => Direct3D9::D3DFILTER_LINEARMIPNEAREST,
//             (FilterMode::Nearest, FilterMode::Linear) => Direct3D9::D3DFILTER_MIPNEAREST,
//             _ => Direct3D9::D3DFILTER_MIPNEAREST
//         }
//     }
// }
