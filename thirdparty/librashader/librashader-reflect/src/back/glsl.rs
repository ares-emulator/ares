use crate::back::targets::GLSL;
use crate::back::{CompileShader, CompilerBackend, FromCompilation};
use crate::error::ShaderReflectError;
use crate::front::SpirvCompilation;
use crate::reflect::cross::{CompiledProgram, SpirvCross};
use crate::reflect::ReflectShader;

/// The GLSL version to target.
pub use spirv_cross::glsl::Version as GlslVersion;

use crate::reflect::cross::glsl::GlslReflect;

/// The context for a GLSL compilation via spirv-cross.
pub struct CrossGlslContext {
    /// A map of bindings of sampler names to binding locations.
    pub sampler_bindings: Vec<(String, u32)>,
    /// The compiled program artifact after compilation.
    pub artifact: CompiledProgram<spirv_cross::glsl::Target>,
}

impl FromCompilation<SpirvCompilation, SpirvCross> for GLSL {
    type Target = GLSL;
    type Options = GlslVersion;
    type Context = CrossGlslContext;
    type Output = impl CompileShader<Self::Target, Options = GlslVersion, Context = Self::Context>
        + ReflectShader;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        Ok(CompilerBackend {
            backend: GlslReflect::try_from(&compile)?,
        })
    }
}
