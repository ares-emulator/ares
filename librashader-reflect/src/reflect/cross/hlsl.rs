use crate::back::hlsl::{CrossHlslContext, HlslBufferAssignment, HlslBufferAssignments};
use crate::back::targets::HLSL;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::cross::{CompiledProgram, CrossReflect};
use spirv::Decoration;

use spirv_cross2::compile::hlsl::HlslShaderModel;
use spirv_cross2::compile::CompilableTarget;
use spirv_cross2::reflect::{DecorationValue, ResourceType};
use spirv_cross2::{targets, SpirvCrossError};

pub(crate) type HlslReflect = CrossReflect<targets::Hlsl>;

impl CompileShader<HLSL> for CrossReflect<targets::Hlsl> {
    type Options = Option<HlslShaderModel>;
    type Context = CrossHlslContext;

    fn compile(
        mut self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, CrossHlslContext>, ShaderCompileError> {
        let sm = options.unwrap_or(HlslShaderModel::ShaderModel5_0);

        let mut options = targets::Hlsl::options();
        options.shader_model = sm;

        // todo: options

        let vertex_resources = self.vertex.shader_resources()?;
        let fragment_resources = self.fragment.shader_resources()?;

        let mut vertex_buffer_assignment = HlslBufferAssignments::default();
        let mut fragment_buffer_assignment = HlslBufferAssignments::default();

        let mut vertex_ubo = vertex_resources.resources_for_type(ResourceType::UniformBuffer)?;
        if vertex_ubo.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }

        if let Some(buf) = vertex_ubo.next() {
            vertex_buffer_assignment.ubo = Some(HlslBufferAssignment {
                name: buf.name.to_string(),
                id: buf.id.id(),
            })
        }

        let mut vertex_pcb = vertex_resources.resources_for_type(ResourceType::PushConstant)?;
        if vertex_pcb.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }
        if let Some(buf) = vertex_pcb.next() {
            vertex_buffer_assignment.push = Some(HlslBufferAssignment {
                name: buf.name.to_string(),
                id: buf.id.id(),
            })
        }

        let mut fragment_ubo =
            fragment_resources.resources_for_type(ResourceType::UniformBuffer)?;
        if fragment_ubo.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }

        if let Some(buf) = fragment_ubo.next() {
            fragment_buffer_assignment.ubo = Some(HlslBufferAssignment {
                name: buf.name.to_string(),
                id: buf.id.id(),
            })
        }

        let mut fragment_pcb = fragment_resources.resources_for_type(ResourceType::PushConstant)?;
        if fragment_pcb.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }

        if let Some(buf) = fragment_pcb.next() {
            fragment_buffer_assignment.push = Some(HlslBufferAssignment {
                name: buf.name.to_string(),
                id: buf.id.id(),
            })
        }

        if sm == HlslShaderModel::ShaderModel3_0 {
            for res in fragment_resources.resources_for_type(ResourceType::SampledImage)? {
                let Some(DecorationValue::Literal(binding)) =
                    self.fragment.decoration(res.id, Decoration::Binding)?
                else {
                    continue;
                };
                self.fragment
                    .set_name(res.id, format!("LIBRA_SAMPLER2D_{binding}"))?;
                // self.fragment
                //     .unset_decoration(res.id, Decoration::Binding)?;
            }
        }

        let vertex_compiled = self.vertex.compile(&options)?;
        let fragment_compiled = self.fragment.compile(&options)?;

        Ok(ShaderCompilerOutput {
            vertex: vertex_compiled.to_string(),
            fragment: fragment_compiled.to_string(),
            context: CrossHlslContext {
                artifact: CompiledProgram {
                    vertex: vertex_compiled,
                    fragment: fragment_compiled,
                },

                vertex_buffers: vertex_buffer_assignment,
                fragment_buffers: fragment_buffer_assignment,
            },
        })
    }

    fn compile_boxed(
        self: Box<Self>,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, Self::Context>, ShaderCompileError> {
        <CrossReflect<targets::Hlsl> as CompileShader<HLSL>>::compile(*self, options)
    }
}
