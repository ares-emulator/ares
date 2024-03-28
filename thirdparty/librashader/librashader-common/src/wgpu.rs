use crate::{FilterMode, ImageFormat, Size, WrapMode};

impl From<ImageFormat> for Option<wgpu_types::TextureFormat> {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => None,
            ImageFormat::R8Unorm => Some(wgpu_types::TextureFormat::R8Unorm),
            ImageFormat::R8Uint => Some(wgpu_types::TextureFormat::R8Uint),
            ImageFormat::R8Sint => Some(wgpu_types::TextureFormat::R8Sint),
            ImageFormat::R8G8Unorm => Some(wgpu_types::TextureFormat::Rg8Unorm),
            ImageFormat::R8G8Uint => Some(wgpu_types::TextureFormat::Rg8Uint),
            ImageFormat::R8G8Sint => Some(wgpu_types::TextureFormat::Rg8Sint),
            ImageFormat::R8G8B8A8Unorm => Some(wgpu_types::TextureFormat::Rgba8Unorm),
            ImageFormat::R8G8B8A8Uint => Some(wgpu_types::TextureFormat::Rgba8Uint),
            ImageFormat::R8G8B8A8Sint => Some(wgpu_types::TextureFormat::Rgba8Sint),
            ImageFormat::R8G8B8A8Srgb => Some(wgpu_types::TextureFormat::Rgba8UnormSrgb),
            ImageFormat::A2B10G10R10UnormPack32 => Some(wgpu_types::TextureFormat::Rgb10a2Unorm),
            ImageFormat::A2B10G10R10UintPack32 => Some(wgpu_types::TextureFormat::Rgb10a2Uint),
            ImageFormat::R16Uint => Some(wgpu_types::TextureFormat::R16Uint),
            ImageFormat::R16Sint => Some(wgpu_types::TextureFormat::R16Sint),
            ImageFormat::R16Sfloat => Some(wgpu_types::TextureFormat::R16Float),
            ImageFormat::R16G16Uint => Some(wgpu_types::TextureFormat::Rg16Uint),
            ImageFormat::R16G16Sint => Some(wgpu_types::TextureFormat::Rg16Sint),
            ImageFormat::R16G16Sfloat => Some(wgpu_types::TextureFormat::Rg16Float),
            ImageFormat::R16G16B16A16Uint => Some(wgpu_types::TextureFormat::Rgba16Uint),
            ImageFormat::R16G16B16A16Sint => Some(wgpu_types::TextureFormat::Rgba16Sint),
            ImageFormat::R16G16B16A16Sfloat => Some(wgpu_types::TextureFormat::Rgba16Float),
            ImageFormat::R32Uint => Some(wgpu_types::TextureFormat::R32Uint),
            ImageFormat::R32Sint => Some(wgpu_types::TextureFormat::R32Sint),
            ImageFormat::R32Sfloat => Some(wgpu_types::TextureFormat::R32Float),
            ImageFormat::R32G32Uint => Some(wgpu_types::TextureFormat::Rg32Uint),
            ImageFormat::R32G32Sint => Some(wgpu_types::TextureFormat::Rg32Sint),
            ImageFormat::R32G32Sfloat => Some(wgpu_types::TextureFormat::Rg32Float),
            ImageFormat::R32G32B32A32Uint => Some(wgpu_types::TextureFormat::Rgba32Uint),
            ImageFormat::R32G32B32A32Sint => Some(wgpu_types::TextureFormat::Rgba32Sint),
            ImageFormat::R32G32B32A32Sfloat => Some(wgpu_types::TextureFormat::Rgba32Float),
        }
    }
}

impl From<wgpu_types::TextureFormat> for ImageFormat {
    fn from(format: wgpu_types::TextureFormat) -> Self {
        match format {
            wgpu_types::TextureFormat::R8Unorm => ImageFormat::R8Unorm,
            wgpu_types::TextureFormat::R8Uint => ImageFormat::R8Uint,
            wgpu_types::TextureFormat::R8Sint => ImageFormat::R8Sint,
            wgpu_types::TextureFormat::Rg8Unorm => ImageFormat::R8G8Unorm,
            wgpu_types::TextureFormat::Rg8Uint => ImageFormat::R8G8Uint,
            wgpu_types::TextureFormat::Rg8Sint => ImageFormat::R8G8Sint,
            wgpu_types::TextureFormat::Rgba8Unorm => ImageFormat::R8G8B8A8Unorm,
            wgpu_types::TextureFormat::Rgba8Uint => ImageFormat::R8G8B8A8Uint,
            wgpu_types::TextureFormat::Rgba8Sint => ImageFormat::R8G8B8A8Sint,
            wgpu_types::TextureFormat::Rgba8UnormSrgb => ImageFormat::R8G8B8A8Srgb,
            wgpu_types::TextureFormat::Rgb10a2Unorm => ImageFormat::A2B10G10R10UnormPack32,
            wgpu_types::TextureFormat::Rgb10a2Uint => ImageFormat::A2B10G10R10UintPack32,
            wgpu_types::TextureFormat::R16Uint => ImageFormat::R16Uint,
            wgpu_types::TextureFormat::R16Sint => ImageFormat::R16Sint,
            wgpu_types::TextureFormat::R16Float => ImageFormat::R16Sfloat,
            wgpu_types::TextureFormat::Rg16Uint => ImageFormat::R16G16Uint,
            wgpu_types::TextureFormat::Rg16Sint => ImageFormat::R16G16Sint,
            wgpu_types::TextureFormat::Rg16Float => ImageFormat::R16G16Sfloat,
            wgpu_types::TextureFormat::Rgba16Uint => ImageFormat::R16G16B16A16Uint,
            wgpu_types::TextureFormat::Rgba16Sint => ImageFormat::R16G16B16A16Sint,
            wgpu_types::TextureFormat::Rgba16Float => ImageFormat::R16G16B16A16Sfloat,
            wgpu_types::TextureFormat::R32Uint => ImageFormat::R32Uint,
            wgpu_types::TextureFormat::R32Sint => ImageFormat::R32Sint,
            wgpu_types::TextureFormat::R32Float => ImageFormat::R32Sfloat,
            wgpu_types::TextureFormat::Rg32Uint => ImageFormat::R32G32Uint,
            wgpu_types::TextureFormat::Rg32Sint => ImageFormat::R32G32Sint,
            wgpu_types::TextureFormat::Rg32Float => ImageFormat::R32G32Sfloat,
            wgpu_types::TextureFormat::Rgba32Uint => ImageFormat::R32G32B32A32Uint,
            wgpu_types::TextureFormat::Rgba32Sint => ImageFormat::R32G32B32A32Sint,
            wgpu_types::TextureFormat::Rgba32Float => ImageFormat::R32G32B32A32Sfloat,
            _ => ImageFormat::Unknown,
        }
    }
}

impl From<Option<wgpu_types::TextureFormat>> for ImageFormat {
    fn from(format: Option<wgpu_types::TextureFormat>) -> Self {
        let Some(format) = format else {
            return ImageFormat::Unknown;
        };
        ImageFormat::from(format)
    }
}

impl From<wgpu_types::Extent3d> for Size<u32> {
    fn from(value: wgpu_types::Extent3d) -> Self {
        Size {
            width: value.width,
            height: value.height,
        }
    }
}

impl From<FilterMode> for wgpu_types::FilterMode {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => wgpu_types::FilterMode::Linear,
            FilterMode::Nearest => wgpu_types::FilterMode::Nearest,
        }
    }
}

impl From<WrapMode> for wgpu_types::AddressMode {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => wgpu_types::AddressMode::ClampToBorder,
            WrapMode::ClampToEdge => wgpu_types::AddressMode::ClampToEdge,
            WrapMode::Repeat => wgpu_types::AddressMode::Repeat,
            WrapMode::MirroredRepeat => wgpu_types::AddressMode::MirrorRepeat,
        }
    }
}

impl From<Size<u32>> for wgpu_types::Extent3d {
    fn from(value: Size<u32>) -> Self {
        wgpu_types::Extent3d {
            width: value.width,
            height: value.height,
            depth_or_array_layers: 1,
        }
    }
}
