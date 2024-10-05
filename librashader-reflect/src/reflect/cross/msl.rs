use crate::back::msl::CrossMslContext;
use crate::back::targets::MSL;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::cross::{CompiledProgram, CrossReflect};

use spirv::Decoration;
use spirv_cross2::compile::msl::{BindTarget, ResourceBinding};
use spirv_cross2::compile::{msl, CompilableTarget};
use spirv_cross2::reflect::{DecorationValue, ResourceType};
use spirv_cross2::{targets, Compiler};

pub(crate) type MslReflect = CrossReflect<targets::Msl>;

impl CompileShader<MSL> for CrossReflect<targets::Msl> {
    type Options = Option<msl::MslVersion>;
    type Context = CrossMslContext;

    fn compile(
        mut self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, CrossMslContext>, ShaderCompileError> {
        let version = options.unwrap_or(msl::MslVersion::new(2, 0, 0));
        let mut options = targets::Msl::options();
        options.version = version;

        fn set_bindings(
            ast: &mut Compiler<targets::Msl>,
            stage: spirv::ExecutionModel,
        ) -> Result<(), ShaderCompileError> {
            let resources = ast.shader_resources()?;
            for resource in resources.resources_for_type(ResourceType::PushConstant)? {
                let Some(DecorationValue::Literal(buffer)) =
                    ast.decoration(resource.id, Decoration::Binding)?
                else {
                    continue;
                };

                ast.add_resource_binding(
                    stage,
                    ResourceBinding::PushConstantBuffer,
                    &BindTarget {
                        buffer,
                        texture: 0,
                        sampler: 0,
                        count: None,
                    },
                )?
            }

            let ubos = resources.resources_for_type(ResourceType::UniformBuffer)?;
            let sampled = resources.resources_for_type(ResourceType::SampledImage)?;

            for resource in ubos.chain(sampled) {
                let Some(DecorationValue::Literal(binding)) =
                    ast.decoration(resource.id, Decoration::Binding)?
                else {
                    continue;
                };

                let Some(DecorationValue::Literal(desc_set)) =
                    ast.decoration(resource.id, Decoration::DescriptorSet)?
                else {
                    continue;
                };

                let overridden = BindTarget {
                    buffer: binding,
                    texture: binding,
                    sampler: binding,
                    count: None,
                };

                ast.add_resource_binding(
                    stage,
                    ResourceBinding::Qualified {
                        set: desc_set,
                        binding,
                    },
                    &overridden,
                )?
            }

            Ok(())
        }
        set_bindings(&mut self.vertex, spirv::ExecutionModel::Vertex)?;

        set_bindings(&mut self.fragment, spirv::ExecutionModel::Fragment)?;

        let vertex_compiled = self.vertex.compile(&options)?;
        let fragment_compiled = self.fragment.compile(&options)?;

        Ok(ShaderCompilerOutput {
            vertex: vertex_compiled.to_string(),
            fragment: fragment_compiled.to_string(),
            context: CrossMslContext {
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
        <CrossReflect<targets::Msl> as CompileShader<MSL>>::compile(*self, options)
    }
}

#[cfg(test)]
mod test {
    use crate::back::targets::{MSL, WGSL};
    use crate::back::{CompileShader, FromCompilation};
    use crate::reflect::cross::SpirvCross;
    use crate::reflect::semantics::{Semantic, ShaderSemantics, UniformSemantic, UniqueSemantics};
    use crate::reflect::ReflectShader;
    use bitflags::Flags;
    use librashader_common::map::{FastHashMap, ShortString};
    use librashader_preprocess::ShaderSource;

    use spirv_cross2::compile::msl::MslVersion;

    #[test]
    pub fn test_into() {
        // let result = ShaderSource::load("../test/shaders_slang/crt/shaders/crt-royale/src/crt-royale-scanlines-horizontal-apply-mask.slang").unwrap();
        // let result = ShaderSource::load("../test/shaders_slang/crt/shaders/crt-royale/src/crt-royale-scanlines-horizontal-apply-mask.slang").unwrap();
        let result = ShaderSource::load("../test/basic.slang").unwrap();

        let mut uniform_semantics: FastHashMap<ShortString, UniformSemantic> = Default::default();

        for (_index, param) in result.parameters.iter().enumerate() {
            uniform_semantics.insert(
                param.1.id.clone(),
                UniformSemantic::Unique(Semantic {
                    semantics: UniqueSemantics::FloatParameter,
                    index: (),
                }),
            );
        }

        let compilation = crate::front::SpirvCompilation::try_from(&result).unwrap();

        let mut msl =
            <MSL as FromCompilation<_, SpirvCross>>::from_compilation(compilation).unwrap();

        msl.reflect(
            0,
            &ShaderSemantics {
                uniform_semantics,
                texture_semantics: Default::default(),
            },
        )
        .expect("");

        let compiled = msl.compile(Some(MslVersion::new(2, 0, 0))).unwrap();

        println!("{}", compiled.vertex);
    }
}
