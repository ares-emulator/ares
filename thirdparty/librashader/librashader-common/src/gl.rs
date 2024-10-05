use crate::{FilterMode, ImageFormat, WrapMode};

impl From<ImageFormat> for u32 {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => 0,
            ImageFormat::R8Unorm => glow::R8,
            ImageFormat::R8Uint => glow::R8UI,
            ImageFormat::R8Sint => glow::R8I,
            ImageFormat::R8G8Unorm => glow::RG8,
            ImageFormat::R8G8Uint => glow::RG8UI,
            ImageFormat::R8G8Sint => glow::RG8I,
            ImageFormat::R8G8B8A8Unorm => glow::RGBA8,
            ImageFormat::R8G8B8A8Uint => glow::RGBA8UI,
            ImageFormat::R8G8B8A8Sint => glow::RGBA8I,
            ImageFormat::R8G8B8A8Srgb => glow::SRGB8_ALPHA8,
            ImageFormat::A2B10G10R10UnormPack32 => glow::RGB10_A2,
            ImageFormat::A2B10G10R10UintPack32 => glow::RGB10_A2UI,
            ImageFormat::R16Uint => glow::R16UI,
            ImageFormat::R16Sint => glow::R16I,
            ImageFormat::R16Sfloat => glow::R16F,
            ImageFormat::R16G16Uint => glow::RG16UI,
            ImageFormat::R16G16Sint => glow::RG16I,
            ImageFormat::R16G16Sfloat => glow::RG16F,
            ImageFormat::R16G16B16A16Uint => glow::RGBA16UI,
            ImageFormat::R16G16B16A16Sint => glow::RGBA16I,
            ImageFormat::R16G16B16A16Sfloat => glow::RGBA16F,
            ImageFormat::R32Uint => glow::R32UI,
            ImageFormat::R32Sint => glow::R32I,
            ImageFormat::R32Sfloat => glow::R32F,
            ImageFormat::R32G32Uint => glow::RG32UI,
            ImageFormat::R32G32Sint => glow::RG32I,
            ImageFormat::R32G32Sfloat => glow::RG32F,
            ImageFormat::R32G32B32A32Uint => glow::RGBA32UI,
            ImageFormat::R32G32B32A32Sint => glow::RGBA32I,
            ImageFormat::R32G32B32A32Sfloat => glow::RGBA32F,
        }
    }
}

impl From<WrapMode> for i32 {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => glow::CLAMP_TO_BORDER as i32,
            WrapMode::ClampToEdge => glow::CLAMP_TO_EDGE as i32,
            WrapMode::Repeat => glow::REPEAT as i32,
            WrapMode::MirroredRepeat => glow::MIRRORED_REPEAT as i32,
        }
    }
}

impl From<FilterMode> for i32 {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => glow::LINEAR as i32,
            _ => glow::NEAREST as i32,
        }
    }
}

impl FilterMode {
    /// Get the mipmap filtering mode for the given combination.
    pub fn gl_mip(&self, mip: FilterMode) -> u32 {
        match (self, mip) {
            (FilterMode::Linear, FilterMode::Linear) => glow::LINEAR_MIPMAP_LINEAR,
            (FilterMode::Linear, FilterMode::Nearest) => glow::LINEAR_MIPMAP_NEAREST,
            (FilterMode::Nearest, FilterMode::Linear) => glow::NEAREST_MIPMAP_LINEAR,
            _ => glow::NEAREST_MIPMAP_NEAREST,
        }
    }
}
