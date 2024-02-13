use crate::back::glsl::CrossGlslContext;
use crate::back::targets::GLSL;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::cross::{CompiledAst, CompiledProgram, CrossReflect};
use spirv_cross::spirv::Decoration;
use spirv_cross::ErrorCode;

pub(crate) type GlslReflect = CrossReflect<spirv_cross::glsl::Target>;

impl CompileShader<GLSL> for CrossReflect<spirv_cross::glsl::Target> {
    type Options = spirv_cross::glsl::Version;
    type Context = CrossGlslContext;

    fn compile(
        mut self,
        version: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, Self::Context>, ShaderCompileError> {
        let mut options: spirv_cross::glsl::CompilerOptions = Default::default();
        options.version = version;
        options.fragment.default_float_precision = spirv_cross::glsl::Precision::High;
        options.fragment.default_int_precision = spirv_cross::glsl::Precision::High;
        options.enable_420_pack_extension = false;

        self.vertex.set_compiler_options(&options)?;
        self.fragment.set_compiler_options(&options)?;

        let vertex_resources = self.vertex.get_shader_resources()?;
        let fragment_resources = self.fragment.get_shader_resources()?;

        for res in &vertex_resources.stage_outputs {
            // let location = self.vertex.get_decoration(res.id, Decoration::Location)?;
            // self.vertex
            //     .set_name(res.id, &format!("LIBRA_VARYING_{location}"))?;
            self.vertex.unset_decoration(res.id, Decoration::Location)?;
        }
        for res in &fragment_resources.stage_inputs {
            // let location = self.fragment.get_decoration(res.id, Decoration::Location)?;
            // self.fragment
            //     .set_name(res.id, &format!("LIBRA_VARYING_{location}"))?;
            self.fragment
                .unset_decoration(res.id, Decoration::Location)?;
        }

        if vertex_resources.push_constant_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }
        for res in &vertex_resources.push_constant_buffers {
            self.vertex.set_name(res.id, "LIBRA_PUSH_VERTEX_INSTANCE")?;
            self.vertex
                .set_name(res.base_type_id, "LIBRA_PUSH_VERTEX")?;
        }

        // todo: options
        let _flatten = false;

        if vertex_resources.uniform_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }
        for res in &vertex_resources.uniform_buffers {
            // if flatten {
            //     self.vertex.flatten_buffer_block(res.id)?;
            // }
            self.vertex.set_name(res.id, "LIBRA_UBO_VERTEX_INSTANCE")?;
            self.vertex.set_name(res.base_type_id, "LIBRA_UBO_VERTEX")?;
            self.vertex
                .unset_decoration(res.id, Decoration::DescriptorSet)?;
            self.vertex.unset_decoration(res.id, Decoration::Binding)?;
        }

        if fragment_resources.push_constant_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one push constant buffer",
                )),
            ));
        }
        for res in &fragment_resources.push_constant_buffers {
            self.fragment
                .set_name(res.id, "LIBRA_PUSH_FRAGMENT_INSTANCE")?;
            self.fragment
                .set_name(res.base_type_id, "LIBRA_PUSH_FRAGMENT")?;
        }

        if fragment_resources.uniform_buffers.len() > 1 {
            return Err(ShaderCompileError::SpirvCrossCompileError(
                ErrorCode::CompilationError(String::from(
                    "Cannot have more than one uniform buffer",
                )),
            ));
        }

        for res in &fragment_resources.uniform_buffers {
            // if flatten {
            //     self.fragment.flatten_buffer_block(res.id)?;
            // }
            self.fragment
                .set_name(res.id, "LIBRA_UBO_FRAGMENT_INSTANCE")?;
            self.fragment
                .set_name(res.base_type_id, "LIBRA_UBO_FRAGMENT")?;
            self.fragment
                .unset_decoration(res.id, Decoration::DescriptorSet)?;
            self.fragment
                .unset_decoration(res.id, Decoration::Binding)?;
        }

        let mut texture_fixups = Vec::new();
        for res in fragment_resources.sampled_images {
            let binding = self.fragment.get_decoration(res.id, Decoration::Binding)?;
            self.fragment
                .unset_decoration(res.id, Decoration::DescriptorSet)?;
            self.fragment
                .unset_decoration(res.id, Decoration::Binding)?;
            let mut name = res.name;
            name.push('\0');
            texture_fixups.push((name, binding));
        }

        Ok(ShaderCompilerOutput {
            vertex: self.vertex.compile()?,
            fragment: self.fragment.compile()?,
            context: CrossGlslContext {
                sampler_bindings: texture_fixups,
                artifact: CompiledProgram {
                    vertex: CompiledAst(self.vertex),
                    fragment: CompiledAst(self.fragment),
                },
            },
        })
    }
}
