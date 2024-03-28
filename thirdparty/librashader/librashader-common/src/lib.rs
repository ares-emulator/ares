//! Common types and conversions for librashader.

/// OpenGL common conversions.
#[cfg(feature = "opengl")]
pub mod gl;

/// Vulkan common conversions.
#[cfg(feature = "vulkan")]
pub mod vk;

/// WGPU common conversions.
#[cfg(feature = "wgpu")]
pub mod wgpu;

/// DXGI common conversions.
#[cfg(all(target_os = "windows", feature = "dxgi"))]
pub mod dxgi;

/// Direct3D 9 common conversions.
#[cfg(all(target_os = "windows", feature = "d3d9"))]
pub mod d3d9;

/// Direct3D 11 common conversions.
#[cfg(all(target_os = "windows", feature = "d3d11"))]
pub mod d3d11;

/// Direct3D 12 common conversions.
#[cfg(all(target_os = "windows", feature = "d3d12"))]
pub mod d3d12;

#[cfg(all(target_vendor = "apple", feature = "metal"))]
pub mod metal;

mod viewport;

#[doc(hidden)]
pub mod map;

pub use viewport::Viewport;

use num_traits::AsPrimitive;
use std::convert::Infallible;
use std::str::FromStr;

#[repr(u32)]
#[derive(Default, Copy, Clone, Debug, Eq, PartialEq, Hash)]
/// Supported image formats for textures.
pub enum ImageFormat {
    #[default]
    Unknown = 0,

    /* 8-bit */
    R8Unorm,
    R8Uint,
    R8Sint,
    R8G8Unorm,
    R8G8Uint,
    R8G8Sint,
    R8G8B8A8Unorm,
    R8G8B8A8Uint,
    R8G8B8A8Sint,
    R8G8B8A8Srgb,

    /* 10-bit */
    A2B10G10R10UnormPack32,
    A2B10G10R10UintPack32,

    /* 16-bit */
    R16Uint,
    R16Sint,
    R16Sfloat,
    R16G16Uint,
    R16G16Sint,
    R16G16Sfloat,
    R16G16B16A16Uint,
    R16G16B16A16Sint,
    R16G16B16A16Sfloat,

    /* 32-bit */
    R32Uint,
    R32Sint,
    R32Sfloat,
    R32G32Uint,
    R32G32Sint,
    R32G32Sfloat,
    R32G32B32A32Uint,
    R32G32B32A32Sint,
    R32G32B32A32Sfloat,
}

#[repr(i32)]
#[derive(Copy, Clone, Default, Debug, Eq, PartialEq, Hash)]
/// The filtering mode for a texture sampler.
pub enum FilterMode {
    /// Linear filtering.
    Linear = 0,

    #[default]
    /// Nearest-neighbour (point) filtering.
    Nearest,
}

impl FromStr for WrapMode {
    type Err = Infallible;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(match s {
            "clamp_to_border" => WrapMode::ClampToBorder,
            "clamp_to_edge" => WrapMode::ClampToEdge,
            "repeat" => WrapMode::Repeat,
            "mirrored_repeat" => WrapMode::MirroredRepeat,
            _ => WrapMode::ClampToBorder,
        })
    }
}

impl FromStr for FilterMode {
    type Err = Infallible;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(match s {
            "linear" => FilterMode::Linear,
            "nearest" => FilterMode::Nearest,
            _ => FilterMode::Nearest,
        })
    }
}

#[repr(i32)]
#[derive(Copy, Clone, Default, Debug, Eq, PartialEq, Hash)]
/// The wrapping (address) mode for a texture sampler.
pub enum WrapMode {
    #[default]
    /// Clamp txture to border.
    ClampToBorder = 0,
    /// Clamp texture to edge.
    ClampToEdge,
    /// Repeat addressing mode.
    Repeat,
    /// Mirrored repeat addressing mode.
    MirroredRepeat,
}

impl FromStr for ImageFormat {
    type Err = Infallible;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(match s {
            "UNKNOWN" => Self::Unknown,

            "R8_UNORM" => Self::R8Unorm,
            "R8_UINT" => Self::R8Uint,
            "R8_SINT" => Self::R8Sint,
            "R8G8_UNORM" => Self::R8G8Unorm,
            "R8G8_UINT" => Self::R8Uint,
            "R8G8_SINT" => Self::R8G8Sint,
            "R8G8B8A8_UNORM" => Self::R8G8B8A8Unorm,
            "R8G8B8A8_UINT" => Self::R8G8B8A8Uint,
            "R8G8B8A8_SINT" => Self::R8G8B8A8Sint,
            "R8G8B8A8_SRGB" => Self::R8G8B8A8Srgb,

            "A2B10G10R10_UNORM_PACK32" => Self::A2B10G10R10UnormPack32,
            "A2B10G10R10_UINT_PACK32" => Self::A2B10G10R10UintPack32,

            "R16_UINT" => Self::R16Uint,
            "R16_SINT" => Self::R16Sint,
            "R16_SFLOAT" => Self::R16Sfloat,
            "R16G16_UINT" => Self::R16G16Uint,
            "R16G16_SINT" => Self::R16G16Sint,
            "R16G16_SFLOAT" => Self::R16G16Sfloat,
            "R16G16B16A16_UINT" => Self::R16G16B16A16Uint,
            "R16G16B16A16_SINT" => Self::R16G16B16A16Sint,
            "R16G16B16A16_SFLOAT" => Self::R16G16B16A16Sfloat,

            "R32_UINT" => Self::R32Uint,
            "R32_SINT" => Self::R32Sint,
            "R32_SFLOAT" => Self::R32Sfloat,
            "R32G32_UINT" => Self::R32G32Uint,
            "R32G32_SINT" => Self::R32G32Sint,
            "R32G32_SFLOAT" => Self::R32G32Sfloat,
            "R32G32B32A32_UINT" => Self::R32G32B32A32Uint,
            "R32G32B32A32_SINT" => Self::R32G32B32A32Sint,
            "R32G32B32A32_SFLOAT" => Self::R32G32B32A32Sfloat,
            _ => Self::Unknown,
        })
    }
}

/// A size with a width and height.
#[derive(Default, Debug, Copy, Clone, PartialEq, Eq)]
pub struct Size<T> {
    pub width: T,
    pub height: T,
}

impl<T> Size<T> {
    /// Create a new `Size<T>` with the given width and height.
    pub fn new(width: T, height: T) -> Self {
        Size { width, height }
    }
}

impl<T> From<Size<T>> for [f32; 4]
where
    T: Copy + AsPrimitive<f32>,
{
    /// Convert a `Size<T>` to a `vec4` uniform.
    fn from(value: Size<T>) -> Self {
        [
            value.width.as_(),
            value.height.as_(),
            1.0 / value.width.as_(),
            1.0 / value.height.as_(),
        ]
    }
}
