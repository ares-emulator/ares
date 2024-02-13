use crate::back::targets::HLSL;
use crate::back::{CompileShader, CompilerBackend, FromCompilation};
use crate::error::ShaderReflectError;
use crate::front::SpirvCompilation;
use crate::reflect::cross::hlsl::HlslReflect;
use crate::reflect::cross::{CompiledProgram, SpirvCross};
use crate::reflect::ReflectShader;

/// The HLSL shader model version to target.
pub use spirv_cross::hlsl::ShaderModel as HlslShaderModel;

/// The context for a HLSL compilation via spirv-cross.
pub struct CrossHlslContext {
    /// The compiled HLSL program.
    pub artifact: CompiledProgram<spirv_cross::hlsl::Target>,
}

impl FromCompilation<SpirvCompilation, SpirvCross> for HLSL {
    type Target = HLSL;
    type Options = Option<HlslShaderModel>;
    type Context = CrossHlslContext;
    type Output = impl CompileShader<Self::Target, Options = Self::Options, Context = Self::Context>
        + ReflectShader;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        Ok(CompilerBackend {
            backend: HlslReflect::try_from(&compile)?,
        })
    }
}
