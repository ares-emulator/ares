use crate::back::hlsl::CrossHlslContext;
use crate::back::targets::HLSL;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::cross::{CompiledAst, CompiledProgram, CrossReflect};
use spirv_cross::hlsl::ShaderModel as HlslShaderModel;

pub(crate) type HlslReflect = CrossReflect<spirv_cross::hlsl::Target>;

impl CompileShader<HLSL> for CrossReflect<spirv_cross::hlsl::Target> {
    type Options = Option<HlslShaderModel>;
    type Context = CrossHlslContext;

    fn compile(
        mut self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, CrossHlslContext>, ShaderCompileError> {
        let sm = options.unwrap_or(HlslShaderModel::V5_0);
        let mut options = spirv_cross::hlsl::CompilerOptions::default();
        options.shader_model = sm;

        self.vertex.set_compiler_options(&options)?;
        self.fragment.set_compiler_options(&options)?;

        Ok(ShaderCompilerOutput {
            vertex: self.vertex.compile()?,
            fragment: self.fragment.compile()?,
            context: CrossHlslContext {
                artifact: CompiledProgram {
                    vertex: CompiledAst(self.vertex),
                    fragment: CompiledAst(self.fragment),
                },
            },
        })
    }
}
