use crate::back::glsl::CrossGlslContext;
use crate::back::targets::GLSL;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::cross::{CompiledProgram, CrossReflect};
use spirv::Decoration;

use spirv_cross2::compile::CompilableTarget;
use spirv_cross2::reflect::{DecorationValue, ResourceType};
use spirv_cross2::{targets, SpirvCrossError};

pub(crate) type GlslReflect = CrossReflect<targets::Glsl>;

impl CompileShader<GLSL> for CrossReflect<targets::Glsl> {
    type Options = spirv_cross2::compile::glsl::GlslVersion;
    type Context = CrossGlslContext;

    fn compile(
        mut self,
        version: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, Self::Context>, ShaderCompileError> {
        let mut options = targets::Glsl::options();

        options.version = version;

        options.es_default_float_precision_highp = true;
        options.es_default_int_precision_highp = true;
        options.enable_420pack_extension = false;

        let vertex_resources = self.vertex.shader_resources()?;
        let fragment_resources = self.fragment.shader_resources()?;

        for res in vertex_resources.resources_for_type(ResourceType::StageOutput)? {
            // let location = self.vertex.get_decoration(res.id, Decoration::Location)?;
            // self.vertex
            //     .set_name(res.id, &format!("LIBRA_VARYING_{location}"))?;
            self.vertex
                .set_decoration(res.id, Decoration::Location, DecorationValue::unset())?;
        }
        for res in fragment_resources.resources_for_type(ResourceType::StageInput)? {
            // let location = self.fragment.get_decoration(res.id, Decoration::Location)?;
            // self.fragment
            //     .set_name(res.id, &format!("LIBRA_VARYING_{location}"))?;
            self.fragment
                .set_decoration(res.id, Decoration::Location, DecorationValue::unset())?;
        }

        let vertex_pcb = vertex_resources.resources_for_type(ResourceType::PushConstant)?;
        if vertex_pcb.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }
        for res in vertex_pcb {
            self.vertex
                .set_name(res.id, c"LIBRA_PUSH_VERTEX_INSTANCE")?;
            self.vertex
                .set_name(res.base_type_id, c"LIBRA_PUSH_VERTEX")?;
        }

        // todo: options
        let _flatten = false;

        let vertex_ubo = vertex_resources.resources_for_type(ResourceType::UniformBuffer)?;
        if vertex_ubo.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }
        for res in vertex_ubo {
            // if flatten {
            //     self.vertex.flatten_buffer_block(res.id)?;
            // }
            self.vertex.set_name(res.id, c"LIBRA_UBO_VERTEX_INSTANCE")?;
            self.vertex
                .set_name(res.base_type_id, c"LIBRA_UBO_VERTEX")?;
            self.vertex.set_decoration(
                res.id,
                Decoration::DescriptorSet,
                DecorationValue::unset(),
            )?;
            self.vertex
                .set_decoration(res.id, Decoration::Binding, DecorationValue::unset())?;
        }

        let fragment_pcb = fragment_resources.resources_for_type(ResourceType::PushConstant)?;
        if fragment_pcb.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }

        for res in fragment_pcb {
            self.fragment
                .set_name(res.id, c"LIBRA_PUSH_FRAGMENT_INSTANCE")?;
            self.fragment
                .set_name(res.base_type_id, c"LIBRA_PUSH_FRAGMENT")?;
        }

        let fragment_ubo = fragment_resources.resources_for_type(ResourceType::UniformBuffer)?;
        if fragment_ubo.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                SpirvCrossError::InvalidArgument(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }

        for res in fragment_ubo {
            // if flatten {
            //     self.fragment.flatten_buffer_block(res.id)?;
            // }
            self.fragment
                .set_name(res.id, c"LIBRA_UBO_FRAGMENT_INSTANCE")?;
            self.fragment
                .set_name(res.base_type_id, c"LIBRA_UBO_FRAGMENT")?;
            self.fragment.set_decoration(
                res.id,
                Decoration::DescriptorSet,
                DecorationValue::unset(),
            )?;
            self.fragment
                .set_decoration(res.id, Decoration::Binding, DecorationValue::unset())?;
        }

        let mut texture_fixups = Vec::new();

        for res in fragment_resources.resources_for_type(ResourceType::SampledImage)? {
            let Some(DecorationValue::Literal(binding)) =
                self.fragment.decoration(res.id, Decoration::Binding)?
            else {
                continue;
            };

            self.fragment.set_decoration(
                res.id,
                Decoration::DescriptorSet,
                DecorationValue::unset(),
            )?;
            self.fragment
                .set_decoration(res.id, Decoration::Binding, DecorationValue::unset())?;
            let name = res.name.to_string();
            texture_fixups.push((name, binding));
        }

        let vertex_compiled = self.vertex.compile(&options)?;
        let fragment_compiled = self.fragment.compile(&options)?;

        Ok(ShaderCompilerOutput {
            vertex: vertex_compiled.to_string(),
            fragment: fragment_compiled.to_string(),
            context: CrossGlslContext {
                sampler_bindings: texture_fixups,
                artifact: CompiledProgram {
                    vertex: vertex_compiled,
                    fragment: fragment_compiled,
                },
            },
        })
    }

    fn compile_boxed(
        self: Box<Self>,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, Self::Context>, ShaderCompileError> {
        <CrossReflect<targets::Glsl> as CompileShader<GLSL>>::compile(*self, options)
    }
}
