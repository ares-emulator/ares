use crate::back::targets::GLSL;
use crate::back::{CompileReflectShader, CompilerBackend, FromCompilation};
use crate::error::ShaderReflectError;
use crate::front::SpirvCompilation;
use crate::reflect::cross::{CompiledProgram, SpirvCross};

/// The GLSL version to target.
pub use spirv_cross2::compile::glsl::GlslVersion;

use crate::reflect::cross::glsl::GlslReflect;

/// The context for a GLSL compilation via spirv-cross.
pub struct CrossGlslContext {
    /// A map of bindings of sampler names to binding locations.
    pub sampler_bindings: Vec<(String, u32)>,
    /// The compiled program artifact after compilation.
    pub artifact: CompiledProgram<spirv_cross2::targets::Glsl>,
}

#[cfg(not(feature = "stable"))]
impl FromCompilation<SpirvCompilation, SpirvCross> for GLSL {
    type Target = GLSL;
    type Options = GlslVersion;
    type Context = CrossGlslContext;
    type Output = impl CompileReflectShader<Self::Target, SpirvCompilation, SpirvCross>;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        Ok(CompilerBackend {
            backend: GlslReflect::try_from(&compile)?,
        })
    }
}

#[cfg(feature = "stable")]
impl FromCompilation<SpirvCompilation, SpirvCross> for GLSL {
    type Target = GLSL;
    type Options = GlslVersion;
    type Context = CrossGlslContext;
    type Output = Box<dyn CompileReflectShader<Self::Target, SpirvCompilation, SpirvCross> + Send>;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        Ok(CompilerBackend {
            backend: Box::new(GlslReflect::try_from(&compile)?),
        })
    }
}
