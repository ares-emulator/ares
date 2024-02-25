use crate::{FilterMode, ImageFormat, Size, WrapMode};
use icrate::Metal;

impl From<ImageFormat> for Metal::MTLPixelFormat {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => 0 as Metal::MTLPixelFormat,
            ImageFormat::R8Unorm => Metal::MTLPixelFormatR8Unorm,
            ImageFormat::R8Uint => Metal::MTLPixelFormatR8Uint,
            ImageFormat::R8Sint => Metal::MTLPixelFormatR8Sint,
            ImageFormat::R8G8Unorm => Metal::MTLPixelFormatRG8Unorm,
            ImageFormat::R8G8Uint => Metal::MTLPixelFormatRG8Uint,
            ImageFormat::R8G8Sint => Metal::MTLPixelFormatRG8Sint,
            ImageFormat::R8G8B8A8Unorm => Metal::MTLPixelFormatRGBA8Unorm,
            ImageFormat::R8G8B8A8Uint => Metal::MTLPixelFormatRGBA8Uint,
            ImageFormat::R8G8B8A8Sint => Metal::MTLPixelFormatRGBA8Sint,
            ImageFormat::R8G8B8A8Srgb => Metal::MTLPixelFormatRGBA8Unorm_sRGB,
            ImageFormat::A2B10G10R10UnormPack32 => Metal::MTLPixelFormatRGB10A2Unorm,
            ImageFormat::A2B10G10R10UintPack32 => Metal::MTLPixelFormatRGB10A2Uint,
            ImageFormat::R16Uint => Metal::MTLPixelFormatR16Uint,
            ImageFormat::R16Sint => Metal::MTLPixelFormatR16Sint,
            ImageFormat::R16Sfloat => Metal::MTLPixelFormatR16Float,
            ImageFormat::R16G16Uint => Metal::MTLPixelFormatRG16Uint,
            ImageFormat::R16G16Sint => Metal::MTLPixelFormatRG16Sint,
            ImageFormat::R16G16Sfloat => Metal::MTLPixelFormatRG16Float,
            ImageFormat::R16G16B16A16Uint => Metal::MTLPixelFormatRGBA16Uint,
            ImageFormat::R16G16B16A16Sint => Metal::MTLPixelFormatRGBA16Sint,
            ImageFormat::R16G16B16A16Sfloat => Metal::MTLPixelFormatRGBA16Float,
            ImageFormat::R32Uint => Metal::MTLPixelFormatR32Uint,
            ImageFormat::R32Sint => Metal::MTLPixelFormatR32Sint,
            ImageFormat::R32Sfloat => Metal::MTLPixelFormatR32Float,
            ImageFormat::R32G32Uint => Metal::MTLPixelFormatRG32Uint,
            ImageFormat::R32G32Sint => Metal::MTLPixelFormatRG32Sint,
            ImageFormat::R32G32Sfloat => Metal::MTLPixelFormatRG32Float,
            ImageFormat::R32G32B32A32Uint => Metal::MTLPixelFormatRGBA32Uint,
            ImageFormat::R32G32B32A32Sint => Metal::MTLPixelFormatRGBA32Sint,
            ImageFormat::R32G32B32A32Sfloat => Metal::MTLPixelFormatRGBA32Float,
        }
    }
}

impl From<Metal::MTLViewport> for Size<u32> {
    fn from(value: Metal::MTLViewport) -> Self {
        Size {
            width: value.width as u32,
            height: value.height as u32,
        }
    }
}

impl From<Size<u32>> for Metal::MTLViewport {
    fn from(value: Size<u32>) -> Self {
        Metal::MTLViewport {
            originX: 0.0,
            originY: 0.0,
            width: value.width as f64,
            height: value.height as f64,
            znear: -1.0,
            zfar: 1.0,
        }
    }
}

impl From<WrapMode> for Metal::MTLSamplerAddressMode {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => Metal::MTLSamplerAddressModeClampToBorderColor,
            WrapMode::ClampToEdge => Metal::MTLSamplerAddressModeClampToEdge,
            WrapMode::Repeat => Metal::MTLSamplerAddressModeRepeat,
            WrapMode::MirroredRepeat => Metal::MTLSamplerAddressModeMirrorRepeat,
        }
    }
}

impl From<FilterMode> for Metal::MTLSamplerMinMagFilter {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => Metal::MTLSamplerMinMagFilterLinear,
            _ => Metal::MTLSamplerMipFilterNearest,
        }
    }
}

impl FilterMode {
    /// Get the mipmap filtering mode for the given combination.
    pub fn mtl_mip(&self, mip: FilterMode) -> Metal::MTLSamplerMipFilter {
        match self {
            FilterMode::Linear => Metal::MTLSamplerMipFilterLinear,
            FilterMode::Nearest => Metal::MTLSamplerMipFilterNearest,
        }
    }
}
