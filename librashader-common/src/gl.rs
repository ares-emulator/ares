use crate::{FilterMode, ImageFormat, WrapMode};

impl From<ImageFormat> for gl::types::GLenum {
    fn from(format: ImageFormat) -> Self {
        match format {
            ImageFormat::Unknown => 0 as gl::types::GLenum,
            ImageFormat::R8Unorm => gl::R8,
            ImageFormat::R8Uint => gl::R8UI,
            ImageFormat::R8Sint => gl::R8I,
            ImageFormat::R8G8Unorm => gl::RG8,
            ImageFormat::R8G8Uint => gl::RG8UI,
            ImageFormat::R8G8Sint => gl::RG8I,
            ImageFormat::R8G8B8A8Unorm => gl::RGBA8,
            ImageFormat::R8G8B8A8Uint => gl::RGBA8UI,
            ImageFormat::R8G8B8A8Sint => gl::RGBA8I,
            ImageFormat::R8G8B8A8Srgb => gl::SRGB8_ALPHA8,
            ImageFormat::A2B10G10R10UnormPack32 => gl::RGB10_A2,
            ImageFormat::A2B10G10R10UintPack32 => gl::RGB10_A2UI,
            ImageFormat::R16Uint => gl::R16UI,
            ImageFormat::R16Sint => gl::R16I,
            ImageFormat::R16Sfloat => gl::R16F,
            ImageFormat::R16G16Uint => gl::RG16UI,
            ImageFormat::R16G16Sint => gl::RG16I,
            ImageFormat::R16G16Sfloat => gl::RG16F,
            ImageFormat::R16G16B16A16Uint => gl::RGBA16UI,
            ImageFormat::R16G16B16A16Sint => gl::RGBA16I,
            ImageFormat::R16G16B16A16Sfloat => gl::RGBA16F,
            ImageFormat::R32Uint => gl::R32UI,
            ImageFormat::R32Sint => gl::R32I,
            ImageFormat::R32Sfloat => gl::R32F,
            ImageFormat::R32G32Uint => gl::RG32UI,
            ImageFormat::R32G32Sint => gl::RG32I,
            ImageFormat::R32G32Sfloat => gl::RG32F,
            ImageFormat::R32G32B32A32Uint => gl::RGBA32UI,
            ImageFormat::R32G32B32A32Sint => gl::RGBA32I,
            ImageFormat::R32G32B32A32Sfloat => gl::RGBA32F,
        }
    }
}

impl From<WrapMode> for gl::types::GLenum {
    fn from(value: WrapMode) -> Self {
        match value {
            WrapMode::ClampToBorder => gl::CLAMP_TO_BORDER,
            WrapMode::ClampToEdge => gl::CLAMP_TO_EDGE,
            WrapMode::Repeat => gl::REPEAT,
            WrapMode::MirroredRepeat => gl::MIRRORED_REPEAT,
        }
    }
}

impl From<FilterMode> for gl::types::GLenum {
    fn from(value: FilterMode) -> Self {
        match value {
            FilterMode::Linear => gl::LINEAR,
            _ => gl::NEAREST,
        }
    }
}

impl FilterMode {
    /// Get the mipmap filtering mode for the given combination.
    pub fn gl_mip(&self, mip: FilterMode) -> gl::types::GLenum {
        match (self, mip) {
            (FilterMode::Linear, FilterMode::Linear) => gl::LINEAR_MIPMAP_LINEAR,
            (FilterMode::Linear, FilterMode::Nearest) => gl::LINEAR_MIPMAP_NEAREST,
            (FilterMode::Nearest, FilterMode::Linear) => gl::NEAREST_MIPMAP_LINEAR,
            _ => gl::NEAREST_MIPMAP_NEAREST,
        }
    }
}
