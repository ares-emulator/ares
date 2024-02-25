use crate::error::ShaderReflectError;
use semantics::ShaderSemantics;

/// Reflection via spirv-cross.
pub mod cross;

/// Shader semantics and reflection information.
pub mod semantics;

/// Reflection helpers for reflecting and compiling shaders as part of a shader preset.
pub mod presets;

mod helper;

#[cfg(feature = "naga")]
pub mod naga;

/// A trait for compilation outputs that can provide reflection information.
pub trait ReflectShader {
    /// Reflect the shader as the given pass within the shader preset, against the provided
    /// semantic map.
    fn reflect(
        &mut self,
        pass_number: usize,
        semantics: &ShaderSemantics,
    ) -> Result<ShaderReflection, ShaderReflectError>;
}

pub use semantics::ShaderReflection;

#[inline(always)]
/// Give a size aligned to 16 byte boundary
const fn align_uniform_size(size: u32) -> u32 {
    (size + 0xf) & !0xf
}
