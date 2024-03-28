use crate::back::hlsl::{CrossHlslContext, HlslBufferAssignment, HlslBufferAssignments};
use crate::back::targets::HLSL;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::cross::{CompiledAst, CompiledProgram, CrossReflect};
use spirv_cross::hlsl::ShaderModel as HlslShaderModel;
use spirv_cross::spirv::Decoration;
use spirv_cross::ErrorCode;

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

        // todo: options

        let vertex_resources = self.vertex.get_shader_resources()?;
        let fragment_resources = self.fragment.get_shader_resources()?;

        let mut vertex_buffer_assignment = HlslBufferAssignments::default();
        let mut fragment_buffer_assignment = HlslBufferAssignments::default();

        if vertex_resources.uniform_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }

        if let Some(buf) = vertex_resources.uniform_buffers.first() {
            vertex_buffer_assignment.ubo = Some(HlslBufferAssignment {
                name: buf.name.clone(),
                id: buf.id,
            })
        }

        if vertex_resources.push_constant_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }

        if let Some(buf) = vertex_resources.push_constant_buffers.first() {
            vertex_buffer_assignment.push = Some(HlslBufferAssignment {
                name: buf.name.clone(),
                id: buf.id,
            })
        }

        if fragment_resources.uniform_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }

        if let Some(buf) = fragment_resources.uniform_buffers.first() {
            fragment_buffer_assignment.ubo = Some(HlslBufferAssignment {
                name: buf.name.clone(),
                id: buf.id,
            })
        }

        if fragment_resources.push_constant_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }

        if let Some(buf) = fragment_resources.push_constant_buffers.first() {
            fragment_buffer_assignment.push = Some(HlslBufferAssignment {
                name: buf.name.clone(),
                id: buf.id,
            })
        }

        if sm == HlslShaderModel::V3_0 {
            for res in &fragment_resources.sampled_images {
                let binding = self.fragment.get_decoration(res.id, Decoration::Binding)?;
                self.fragment
                    .set_name(res.id, &format!("LIBRA_SAMPLER2D_{binding}"))?;
                // self.fragment
                //     .unset_decoration(res.id, Decoration::Binding)?;
            }
        }

        Ok(ShaderCompilerOutput {
            vertex: self.vertex.compile()?,
            fragment: self.fragment.compile()?,
            context: CrossHlslContext {
                artifact: CompiledProgram {
                    vertex: CompiledAst(self.vertex),
                    fragment: CompiledAst(self.fragment),
                },

                vertex_buffers: vertex_buffer_assignment,
                fragment_buffers: fragment_buffer_assignment,
            },
        })
    }
}
