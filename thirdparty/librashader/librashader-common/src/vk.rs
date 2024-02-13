use crate::{FilterMode, ImageFormat, Size, WrapMode};
use ash::vk;

impl From<ImageFormat> for vk::Format {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => vk::Format::UNDEFINED,
            ImageFormat::R8Unorm => vk::Format::R8_UNORM,
            ImageFormat::R8Uint => vk::Format::R8_UINT,
            ImageFormat::R8Sint => vk::Format::R8_SINT,
            ImageFormat::R8G8Unorm => vk::Format::R8G8_UNORM,
            ImageFormat::R8G8Uint => vk::Format::R8G8_UINT,
            ImageFormat::R8G8Sint => vk::Format::R8G8_SINT,
            ImageFormat::R8G8B8A8Unorm => vk::Format::R8G8B8A8_UNORM,
            ImageFormat::R8G8B8A8Uint => vk::Format::R8G8B8A8_UINT,
            ImageFormat::R8G8B8A8Sint => vk::Format::R8G8B8A8_SINT,
            ImageFormat::R8G8B8A8Srgb => vk::Format::R8G8B8A8_SRGB,
            ImageFormat::A2B10G10R10UnormPack32 => vk::Format::A2B10G10R10_UNORM_PACK32,
            ImageFormat::A2B10G10R10UintPack32 => vk::Format::A2B10G10R10_UINT_PACK32,
            ImageFormat::R16Uint => vk::Format::R16_UINT,
            ImageFormat::R16Sint => vk::Format::R16_SINT,
            ImageFormat::R16Sfloat => vk::Format::R16_SFLOAT,
            ImageFormat::R16G16Uint => vk::Format::R16G16_UINT,
            ImageFormat::R16G16Sint => vk::Format::R16G16_SINT,
            ImageFormat::R16G16Sfloat => vk::Format::R16G16_SFLOAT,
            ImageFormat::R16G16B16A16Uint => vk::Format::R16G16B16A16_UINT,
            ImageFormat::R16G16B16A16Sint => vk::Format::R16G16B16A16_SINT,
            ImageFormat::R16G16B16A16Sfloat => vk::Format::R16G16B16A16_SFLOAT,
            ImageFormat::R32Uint => vk::Format::R32_UINT,
            ImageFormat::R32Sint => vk::Format::R32_SINT,
            ImageFormat::R32Sfloat => vk::Format::R32_SFLOAT,
            ImageFormat::R32G32Uint => vk::Format::R32G32_UINT,
            ImageFormat::R32G32Sint => vk::Format::R32G32_SINT,
            ImageFormat::R32G32Sfloat => vk::Format::R32G32_SFLOAT,
            ImageFormat::R32G32B32A32Uint => vk::Format::R32G32B32A32_UINT,
            ImageFormat::R32G32B32A32Sint => vk::Format::R32G32B32A32_SINT,
            ImageFormat::R32G32B32A32Sfloat => vk::Format::R32G32B32A32_SFLOAT,
        }
    }
}

impl From<vk::Format> for ImageFormat {
    fn from(format: vk::Format) -> Self {
        match format {
            vk::Format::UNDEFINED => ImageFormat::Unknown,
            vk::Format::R8_UNORM => ImageFormat::R8Unorm,
            vk::Format::R8_UINT => ImageFormat::R8Uint,
            vk::Format::R8_SINT => ImageFormat::R8Sint,
            vk::Format::R8G8_UNORM => ImageFormat::R8G8Unorm,
            vk::Format::R8G8_UINT => ImageFormat::R8G8Uint,
            vk::Format::R8G8_SINT => ImageFormat::R8G8Sint,
            vk::Format::R8G8B8A8_UNORM => ImageFormat::R8G8B8A8Unorm,
            vk::Format::R8G8B8A8_UINT => ImageFormat::R8G8B8A8Uint,
            vk::Format::R8G8B8A8_SINT => ImageFormat::R8G8B8A8Sint,
            vk::Format::R8G8B8A8_SRGB => ImageFormat::R8G8B8A8Srgb,
            vk::Format::A2B10G10R10_UNORM_PACK32 => ImageFormat::A2B10G10R10UnormPack32,
            vk::Format::A2B10G10R10_UINT_PACK32 => ImageFormat::A2B10G10R10UintPack32,
            vk::Format::R16_UINT => ImageFormat::R16Uint,
            vk::Format::R16_SINT => ImageFormat::R16Sint,
            vk::Format::R16_SFLOAT => ImageFormat::R16Sfloat,
            vk::Format::R16G16_UINT => ImageFormat::R16G16Uint,
            vk::Format::R16G16_SINT => ImageFormat::R16G16Sint,
            vk::Format::R16G16_SFLOAT => ImageFormat::R16G16Sfloat,
            vk::Format::R16G16B16A16_UINT => ImageFormat::R16G16B16A16Uint,
            vk::Format::R16G16B16A16_SINT => ImageFormat::R16G16B16A16Sint,
            vk::Format::R16G16B16A16_SFLOAT => ImageFormat::R16G16B16A16Sfloat,
            vk::Format::R32_UINT => ImageFormat::R32Uint,
            vk::Format::R32_SINT => ImageFormat::R32Sint,
            vk::Format::R32_SFLOAT => ImageFormat::R32Sfloat,
            vk::Format::R32G32_UINT => ImageFormat::R32G32Uint,
            vk::Format::R32G32_SINT => ImageFormat::R32G32Sint,
            vk::Format::R32G32_SFLOAT => ImageFormat::R32G32Sfloat,
            vk::Format::R32G32B32A32_UINT => ImageFormat::R32G32B32A32Uint,
            vk::Format::R32G32B32A32_SINT => ImageFormat::R32G32B32A32Sint,
            vk::Format::R32G32B32A32_SFLOAT => ImageFormat::R32G32B32A32Sfloat,
            _ => ImageFormat::Unknown,
        }
    }
}

impl From<Size<u32>> for vk::Extent3D {
    fn from(value: Size<u32>) -> Self {
        vk::Extent3D {
            width: value.width,
            height: value.height,
            depth: 1,
        }
    }
}

impl From<Size<u32>> for vk::Extent2D {
    fn from(value: Size<u32>) -> Self {
        vk::Extent2D {
            width: value.width,
            height: value.height,
        }
    }
}

impl From<vk::Extent3D> for Size<u32> {
    fn from(value: vk::Extent3D) -> Self {
        Size {
            width: value.width,
            height: value.height,
        }
    }
}

impl From<vk::Extent2D> for Size<u32> {
    fn from(value: vk::Extent2D) -> Self {
        Size {
            width: value.width,
            height: value.height,
        }
    }
}

impl From<vk::Viewport> for Size<u32> {
    fn from(value: vk::Viewport) -> Self {
        Size {
            width: value.width as u32,
            height: value.height as u32,
        }
    }
}

impl From<&vk::Viewport> for Size<u32> {
    fn from(value: &vk::Viewport) -> Self {
        Size {
            width: value.width as u32,
            height: value.height as u32,
        }
    }
}

impl From<Size<u32>> for vk::Viewport {
    fn from(value: Size<u32>) -> Self {
        vk::Viewport {
            x: 0.0,
            y: 0.0,
            width: value.width as f32,
            height: value.height as f32,
            min_depth: 0.0,
            max_depth: 1.0,
        }
    }
}
impl From<FilterMode> for vk::Filter {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => vk::Filter::LINEAR,
            FilterMode::Nearest => vk::Filter::NEAREST,
        }
    }
}

impl From<FilterMode> for vk::SamplerMipmapMode {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => vk::SamplerMipmapMode::LINEAR,
            FilterMode::Nearest => vk::SamplerMipmapMode::NEAREST,
        }
    }
}

impl From<WrapMode> for vk::SamplerAddressMode {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => vk::SamplerAddressMode::CLAMP_TO_BORDER,
            WrapMode::ClampToEdge => vk::SamplerAddressMode::CLAMP_TO_EDGE,
            WrapMode::Repeat => vk::SamplerAddressMode::REPEAT,
            WrapMode::MirroredRepeat => vk::SamplerAddressMode::MIRRORED_REPEAT,
        }
    }
}
