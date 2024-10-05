use crate::error::ParsePresetError;
use librashader_common::map::ShortString;
use librashader_common::{FilterMode, ImageFormat, WrapMode};
use std::ops::Mul;
use std::path::PathBuf;
use std::str::FromStr;

/// The configuration for a single shader pass.
pub type PassConfig = PathReference<PassMeta>;

/// Configuration options for a lookup texture used in the shader.
pub type TextureConfig = PathReference<TextureMeta>;

/// A reference to a resource on disk.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct PathReference<M> {
    /// The fully qualified path to the resource, often a shader source file or a texture.
    pub path: PathBuf,
    /// Meta information about the resource.
    pub meta: M,
}

/// Meta information about a shader pass.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct PassMeta {
    /// The index of the shader pass relative to its parent preset.
    pub id: i32,
    /// The alias of the shader pass if available.
    pub alias: Option<ShortString>,
    /// The filtering mode that this shader pass should expect.
    pub filter: FilterMode,
    /// The texture addressing (wrap) mode that this shader pass expects.
    pub wrap_mode: WrapMode,
    /// The number to which to wrap the frame count before passing it to the uniforms.
    pub frame_count_mod: u32,
    /// Whether or not this shader pass expects an SRGB framebuffer output.
    pub srgb_framebuffer: bool,
    /// Whether or not this shader pass expects an float framebuffer output.
    pub float_framebuffer: bool,
    /// Whether or not to generate mipmaps for the input texture before passing to the shader.
    pub mipmap_input: bool,
    /// Specifies the scaling of the output framebuffer for this shader pass.
    pub scaling: Scale2D,
}

impl PassMeta {
    /// If the framebuffer expects a different format than what was defined in the
    /// shader source, returns such format.
    #[inline(always)]
    pub fn get_format_override(&self) -> Option<ImageFormat> {
        if self.srgb_framebuffer {
            return Some(ImageFormat::R8G8B8A8Srgb);
        } else if self.float_framebuffer {
            return Some(ImageFormat::R16G16B16A16Sfloat);
        }
        None
    }

    #[inline(always)]
    pub fn get_frame_count(&self, count: usize) -> u32 {
        (if self.frame_count_mod > 0 {
            count % self.frame_count_mod as usize
        } else {
            count
        }) as u32
    }
}

#[repr(i32)]
#[derive(Default, Copy, Clone, Debug)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
/// The scaling type for the shader pass.
pub enum ScaleType {
    #[default]
    /// Scale by the size of the input quad.
    Input = 0,
    /// Scale the framebuffer in absolute units.
    Absolute,
    /// Scale by the size of the viewport.
    Viewport,
    /// Scale by the size of the original input quad.
    Original,
}

/// The scaling factor for framebuffer scaling.
#[derive(Copy, Clone, Debug)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub enum ScaleFactor {
    /// Scale by a fractional float factor.
    Float(f32),
    /// Scale by an absolute factor.
    Absolute(i32),
}

impl Default for ScaleFactor {
    fn default() -> Self {
        ScaleFactor::Float(1.0f32)
    }
}

impl From<ScaleFactor> for f32 {
    fn from(value: ScaleFactor) -> Self {
        match value {
            ScaleFactor::Float(f) => f,
            ScaleFactor::Absolute(f) => f as f32,
        }
    }
}

impl Mul<ScaleFactor> for f32 {
    type Output = f32;

    fn mul(self, rhs: ScaleFactor) -> Self::Output {
        match rhs {
            ScaleFactor::Float(f) => f * self,
            ScaleFactor::Absolute(f) => f as f32 * self,
        }
    }
}

impl Mul<ScaleFactor> for u32 {
    type Output = f32;

    fn mul(self, rhs: ScaleFactor) -> Self::Output {
        match rhs {
            ScaleFactor::Float(f) => f * self as f32,
            ScaleFactor::Absolute(f) => (f as u32 * self) as f32,
        }
    }
}

impl FromStr for ScaleType {
    type Err = ParsePresetError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "source" => Ok(ScaleType::Input),
            "viewport" => Ok(ScaleType::Viewport),
            "absolute" => Ok(ScaleType::Absolute),
            "original" => Ok(ScaleType::Original),
            _ => Err(ParsePresetError::InvalidScaleType(s.to_string())),
        }
    }
}

/// Framebuffer scaling parameters.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct Scaling {
    /// The method to scale the framebuffer with.
    pub scale_type: ScaleType,
    /// The factor to scale by.
    pub factor: ScaleFactor,
}

/// 2D quad scaling parameters.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct Scale2D {
    /// Whether or not this combination of scaling factors is valid.
    pub valid: bool,
    /// Scaling parameters for the X axis.
    pub x: Scaling,
    /// Scaling parameters for the Y axis.
    pub y: Scaling,
}

/// Configuration options for a lookup texture used in the shader.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct TextureMeta {
    /// The name of the texture.
    pub name: ShortString,
    /// The wrap (addressing) mode to use when sampling the texture.
    pub wrap_mode: WrapMode,
    /// The filter mode to use when sampling the texture.
    pub filter_mode: FilterMode,
    /// Whether to generate mipmaps for this texture.
    pub mipmap: bool,
}

/// Configuration options for a shader parameter.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct ParameterMeta {
    /// The name of the parameter.
    pub name: ShortString,
    /// The value it is set to in the preset.
    pub value: f32,
}

/// A shader preset including all specified parameters, textures, and paths to specified shaders.
///
/// A shader preset can be used to create a filter chain runtime instance, or reflected to get
/// parameter metadata.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct ShaderPreset {
    /// Used in legacy GLSL shader semantics. If < 0, no feedback pass is used.
    /// Otherwise, the FBO after pass #N is passed a texture to next frame
    #[cfg(feature = "parse_legacy_glsl")]
    pub feedback_pass: i32,

    /// The number of shaders enabled in the filter chain.
    pub pass_count: i32,
    // Everything is in Vecs because the expect number of values is well below 64.
    /// Preset information for each shader.
    pub passes: Vec<PassConfig>,

    /// Preset information for each texture.
    pub textures: Vec<TextureConfig>,

    /// Preset information for each user parameter.
    pub parameters: Vec<ParameterMeta>,
}
