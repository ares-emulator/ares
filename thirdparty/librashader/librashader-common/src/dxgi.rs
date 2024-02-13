use crate::ImageFormat;
use windows::Win32::Graphics::Dxgi::Common as dxgi;
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT;

impl From<ImageFormat> for dxgi::DXGI_FORMAT {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => dxgi::DXGI_FORMAT_UNKNOWN,
            ImageFormat::R8Unorm => dxgi::DXGI_FORMAT_R8_UNORM,
            ImageFormat::R8Uint => dxgi::DXGI_FORMAT_R8_UINT,
            ImageFormat::R8Sint => dxgi::DXGI_FORMAT_R8_SINT,
            ImageFormat::R8G8Unorm => dxgi::DXGI_FORMAT_R8G8_UNORM,
            ImageFormat::R8G8Uint => dxgi::DXGI_FORMAT_R8G8_UINT,
            ImageFormat::R8G8Sint => dxgi::DXGI_FORMAT_R8G8_SINT,
            ImageFormat::R8G8B8A8Unorm => dxgi::DXGI_FORMAT_R8G8B8A8_UNORM,
            ImageFormat::R8G8B8A8Uint => dxgi::DXGI_FORMAT_R8G8B8A8_UINT,
            ImageFormat::R8G8B8A8Sint => dxgi::DXGI_FORMAT_R8G8B8A8_SINT,
            ImageFormat::R8G8B8A8Srgb => dxgi::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            ImageFormat::A2B10G10R10UnormPack32 => dxgi::DXGI_FORMAT_R10G10B10A2_UNORM,
            ImageFormat::A2B10G10R10UintPack32 => dxgi::DXGI_FORMAT_R10G10B10A2_UINT,
            ImageFormat::R16Uint => dxgi::DXGI_FORMAT_R16_UINT,
            ImageFormat::R16Sint => dxgi::DXGI_FORMAT_R16_SINT,
            ImageFormat::R16Sfloat => dxgi::DXGI_FORMAT_R16_FLOAT,
            ImageFormat::R16G16Uint => dxgi::DXGI_FORMAT_R16G16_UINT,
            ImageFormat::R16G16Sint => dxgi::DXGI_FORMAT_R16G16_SINT,
            ImageFormat::R16G16Sfloat => dxgi::DXGI_FORMAT_R16G16_FLOAT,
            ImageFormat::R16G16B16A16Uint => dxgi::DXGI_FORMAT_R16G16B16A16_UINT,
            ImageFormat::R16G16B16A16Sint => dxgi::DXGI_FORMAT_R16G16B16A16_SINT,
            ImageFormat::R16G16B16A16Sfloat => dxgi::DXGI_FORMAT_R16G16B16A16_FLOAT,
            ImageFormat::R32Uint => dxgi::DXGI_FORMAT_R32_UINT,
            ImageFormat::R32Sint => dxgi::DXGI_FORMAT_R32_SINT,
            ImageFormat::R32Sfloat => dxgi::DXGI_FORMAT_R32_FLOAT,
            ImageFormat::R32G32Uint => dxgi::DXGI_FORMAT_R32G32_UINT,
            ImageFormat::R32G32Sint => dxgi::DXGI_FORMAT_R32G32_SINT,
            ImageFormat::R32G32Sfloat => dxgi::DXGI_FORMAT_R32G32_FLOAT,
            ImageFormat::R32G32B32A32Uint => dxgi::DXGI_FORMAT_R32G32B32A32_UINT,
            ImageFormat::R32G32B32A32Sint => dxgi::DXGI_FORMAT_R32G32B32A32_SINT,
            ImageFormat::R32G32B32A32Sfloat => dxgi::DXGI_FORMAT_R32G32B32A32_FLOAT,
        }
    }
}

impl From<DXGI_FORMAT> for ImageFormat {
    fn from(format: DXGI_FORMAT) -> Self {
        match format {
            dxgi::DXGI_FORMAT_UNKNOWN => ImageFormat::Unknown,
            dxgi::DXGI_FORMAT_R8_UNORM => ImageFormat::R8Unorm,
            dxgi::DXGI_FORMAT_R8_UINT => ImageFormat::R8Uint,
            dxgi::DXGI_FORMAT_R8_SINT => ImageFormat::R8Sint,
            dxgi::DXGI_FORMAT_R8G8_UNORM => ImageFormat::R8G8Unorm,
            dxgi::DXGI_FORMAT_R8G8_UINT => ImageFormat::R8G8Uint,
            dxgi::DXGI_FORMAT_R8G8_SINT => ImageFormat::R8G8Sint,
            dxgi::DXGI_FORMAT_R8G8B8A8_UNORM => ImageFormat::R8G8B8A8Unorm,
            dxgi::DXGI_FORMAT_R8G8B8A8_UINT => ImageFormat::R8G8B8A8Uint,
            dxgi::DXGI_FORMAT_R8G8B8A8_SINT => ImageFormat::R8G8B8A8Sint,
            dxgi::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB => ImageFormat::R8G8B8A8Srgb,
            dxgi::DXGI_FORMAT_R10G10B10A2_UNORM => ImageFormat::A2B10G10R10UnormPack32,
            dxgi::DXGI_FORMAT_R10G10B10A2_UINT => ImageFormat::A2B10G10R10UintPack32,
            dxgi::DXGI_FORMAT_R16_UINT => ImageFormat::R16Uint,
            dxgi::DXGI_FORMAT_R16_SINT => ImageFormat::R16Sint,
            dxgi::DXGI_FORMAT_R16_FLOAT => ImageFormat::R16Sfloat,
            dxgi::DXGI_FORMAT_R16G16_UINT => ImageFormat::R16G16Uint,
            dxgi::DXGI_FORMAT_R16G16_SINT => ImageFormat::R16G16Sint,
            dxgi::DXGI_FORMAT_R16G16_FLOAT => ImageFormat::R16G16Sfloat,
            dxgi::DXGI_FORMAT_R16G16B16A16_UINT => ImageFormat::R16G16B16A16Uint,
            dxgi::DXGI_FORMAT_R16G16B16A16_SINT => ImageFormat::R16G16B16A16Sint,
            dxgi::DXGI_FORMAT_R16G16B16A16_FLOAT => ImageFormat::R16G16B16A16Sfloat,
            dxgi::DXGI_FORMAT_R32_UINT => ImageFormat::R32Uint,
            dxgi::DXGI_FORMAT_R32_SINT => ImageFormat::R32Sint,
            dxgi::DXGI_FORMAT_R32_FLOAT => ImageFormat::R32Sfloat,
            dxgi::DXGI_FORMAT_R32G32_UINT => ImageFormat::R32G32Uint,
            dxgi::DXGI_FORMAT_R32G32_SINT => ImageFormat::R32G32Sint,
            dxgi::DXGI_FORMAT_R32G32_FLOAT => ImageFormat::R32G32Sfloat,
            dxgi::DXGI_FORMAT_R32G32B32A32_UINT => ImageFormat::R32G32B32A32Uint,
            dxgi::DXGI_FORMAT_R32G32B32A32_SINT => ImageFormat::R32G32B32A32Sint,
            dxgi::DXGI_FORMAT_R32G32B32A32_FLOAT => ImageFormat::R32G32B32A32Sfloat,
            _ => ImageFormat::Unknown,
        }
    }
}
