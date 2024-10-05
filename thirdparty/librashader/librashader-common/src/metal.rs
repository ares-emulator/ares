use crate::{FilterMode, GetSize, ImageFormat, Size, WrapMode};
use objc2::runtime::ProtocolObject;
use objc2_metal::{
    MTLPixelFormat, MTLSamplerAddressMode, MTLSamplerMinMagFilter, MTLSamplerMipFilter, MTLTexture,
    MTLViewport,
};

impl From<ImageFormat> for MTLPixelFormat {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => MTLPixelFormat(0),
            ImageFormat::R8Unorm => MTLPixelFormat::R8Unorm,
            ImageFormat::R8Uint => MTLPixelFormat::R8Uint,
            ImageFormat::R8Sint => MTLPixelFormat::R8Sint,
            ImageFormat::R8G8Unorm => MTLPixelFormat::RG8Unorm,
            ImageFormat::R8G8Uint => MTLPixelFormat::RG8Uint,
            ImageFormat::R8G8Sint => MTLPixelFormat::RG8Sint,
            ImageFormat::R8G8B8A8Unorm => MTLPixelFormat::RGBA8Unorm,
            ImageFormat::R8G8B8A8Uint => MTLPixelFormat::RGBA8Uint,
            ImageFormat::R8G8B8A8Sint => MTLPixelFormat::RGBA8Sint,
            ImageFormat::R8G8B8A8Srgb => MTLPixelFormat::RGBA8Unorm_sRGB,
            ImageFormat::A2B10G10R10UnormPack32 => MTLPixelFormat::RGB10A2Unorm,
            ImageFormat::A2B10G10R10UintPack32 => MTLPixelFormat::RGB10A2Uint,
            ImageFormat::R16Uint => MTLPixelFormat::R16Uint,
            ImageFormat::R16Sint => MTLPixelFormat::R16Sint,
            ImageFormat::R16Sfloat => MTLPixelFormat::R16Float,
            ImageFormat::R16G16Uint => MTLPixelFormat::RG16Uint,
            ImageFormat::R16G16Sint => MTLPixelFormat::RG16Sint,
            ImageFormat::R16G16Sfloat => MTLPixelFormat::RG16Float,
            ImageFormat::R16G16B16A16Uint => MTLPixelFormat::RGBA16Uint,
            ImageFormat::R16G16B16A16Sint => MTLPixelFormat::RGBA16Sint,
            ImageFormat::R16G16B16A16Sfloat => MTLPixelFormat::RGBA16Float,
            ImageFormat::R32Uint => MTLPixelFormat::R32Uint,
            ImageFormat::R32Sint => MTLPixelFormat::R32Sint,
            ImageFormat::R32Sfloat => MTLPixelFormat::R32Float,
            ImageFormat::R32G32Uint => MTLPixelFormat::RG32Uint,
            ImageFormat::R32G32Sint => MTLPixelFormat::RG32Sint,
            ImageFormat::R32G32Sfloat => MTLPixelFormat::RG32Float,
            ImageFormat::R32G32B32A32Uint => MTLPixelFormat::RGBA32Uint,
            ImageFormat::R32G32B32A32Sint => MTLPixelFormat::RGBA32Sint,
            ImageFormat::R32G32B32A32Sfloat => MTLPixelFormat::RGBA32Float,
        }
    }
}

impl From<MTLViewport> for Size<u32> {
    fn from(value: MTLViewport) -> Self {
        Size {
            width: value.width as u32,
            height: value.height as u32,
        }
    }
}

impl From<Size<u32>> for MTLViewport {
    fn from(value: Size<u32>) -> Self {
        MTLViewport {
            originX: 0.0,
            originY: 0.0,
            width: value.width as f64,
            height: value.height as f64,
            znear: -1.0,
            zfar: 1.0,
        }
    }
}

impl From<WrapMode> for MTLSamplerAddressMode {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => MTLSamplerAddressMode::ClampToBorderColor,
            WrapMode::ClampToEdge => MTLSamplerAddressMode::ClampToEdge,
            WrapMode::Repeat => MTLSamplerAddressMode::Repeat,
            WrapMode::MirroredRepeat => MTLSamplerAddressMode::MirrorRepeat,
        }
    }
}

impl From<FilterMode> for MTLSamplerMinMagFilter {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => MTLSamplerMinMagFilter::Linear,
            _ => MTLSamplerMinMagFilter::Nearest,
        }
    }
}

impl FilterMode {
    /// Get the mipmap filtering mode for the given combination.
    pub fn mtl_mip(&self, _mip: FilterMode) -> MTLSamplerMipFilter {
        match self {
            FilterMode::Linear => MTLSamplerMipFilter::Linear,
            FilterMode::Nearest => MTLSamplerMipFilter::Nearest,
        }
    }
}

impl GetSize<u32> for ProtocolObject<dyn MTLTexture> {
    type Error = std::convert::Infallible;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        let height = self.height();
        let width = self.width();
        Ok(Size {
            height: height as u32,
            width: width as u32,
        })
    }
}

impl GetSize<u32> for &ProtocolObject<dyn MTLTexture> {
    type Error = std::convert::Infallible;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        let height = self.height();
        let width = self.width();
        Ok(Size {
            height: height as u32,
            width: width as u32,
        })
    }
}
