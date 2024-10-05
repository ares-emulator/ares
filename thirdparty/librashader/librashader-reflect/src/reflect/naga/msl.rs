use crate::back::msl::{MslVersion, NagaMslContext, NagaMslModule};
use crate::back::targets::MSL;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::naga::{NagaLoweringOptions, NagaReflect};
use naga::back::msl::{
    BindSamplerTarget, BindTarget, EntryPointResources, Options, PipelineOptions, TranslationInfo,
};
use naga::valid::{Capabilities, ValidationFlags};
use naga::{Module, TypeInner};

fn msl_version_to_naga_msl(version: MslVersion) -> (u8, u8) {
    (version.major as u8, version.minor as u8)
}

impl CompileShader<MSL> for NagaReflect {
    type Options = Option<crate::back::msl::MslVersion>;
    type Context = NagaMslContext;

    fn compile(
        mut self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, Self::Context>, ShaderCompileError> {
        // https://github.com/libretro/RetroArch/blob/434e94c782af2e4d4277a24b7ed8e5fc54870088/gfx/drivers_shader/slang_process.cpp#L524

        let lang_version = msl_version_to_naga_msl(options.unwrap_or(MslVersion::new(2, 0, 0)));

        let mut vert_options = Options {
            lang_version,
            per_entry_point_map: Default::default(),
            inline_samplers: vec![],
            spirv_cross_compatibility: true,
            fake_missing_bindings: false,
            bounds_check_policies: Default::default(),
            zero_initialize_workgroup_memory: false,
        };

        let mut frag_options = vert_options.clone();

        fn write_msl(
            module: &Module,
            options: Options,
        ) -> Result<(String, TranslationInfo), ShaderCompileError> {
            let mut valid =
                naga::valid::Validator::new(ValidationFlags::all(), Capabilities::empty());
            let info = valid.validate(&module)?;

            let pipeline_options = PipelineOptions {
                allow_and_force_point_size: false,
                vertex_pulling_transform: false,
                vertex_buffer_mappings: vec![],
            };

            let msl = naga::back::msl::write_string(&module, &info, &options, &pipeline_options)?;
            Ok(msl)
        }

        fn generate_bindings(module: &Module) -> EntryPointResources {
            let mut resources = EntryPointResources::default();
            let binding_map = &mut resources.resources;
            // Don't set PCB because they'll be gone after lowering..
            // resources.push_constant_buffer = Some(1u8);

            for (_, variable) in module.global_variables.iter() {
                let Some(binding) = &variable.binding else {
                    continue;
                };

                let Ok(ty) = module.types.get_handle(variable.ty) else {
                    continue;
                };

                match ty.inner {
                    TypeInner::Sampler { .. } => {
                        binding_map.insert(
                            binding.clone(),
                            BindTarget {
                                buffer: None,
                                texture: None,
                                sampler: Some(BindSamplerTarget::Resource(binding.binding as u8)),
                                binding_array_size: None,
                                mutable: false,
                            },
                        );
                    }
                    TypeInner::Struct { .. } => {
                        binding_map.insert(
                            binding.clone(),
                            BindTarget {
                                buffer: Some(binding.binding as u8),
                                texture: None,
                                sampler: None,
                                binding_array_size: None,
                                mutable: false,
                            },
                        );
                    }
                    TypeInner::Image { .. } => {
                        binding_map.insert(
                            binding.clone(),
                            BindTarget {
                                buffer: None,
                                texture: Some(binding.binding as u8),
                                sampler: None,
                                binding_array_size: None,
                                mutable: false,
                            },
                        );
                    }
                    _ => continue,
                }
            }

            resources
        }

        self.do_lowering(&NagaLoweringOptions {
            write_pcb_as_ubo: true,
            sampler_bind_group: 1,
        });

        frag_options
            .per_entry_point_map
            .insert(String::from("main"), generate_bindings(&self.fragment));
        vert_options
            .per_entry_point_map
            .insert(String::from("main"), generate_bindings(&self.vertex));

        let fragment = write_msl(&self.fragment, frag_options)?;
        let vertex = write_msl(&self.vertex, vert_options)?;

        let vertex_binding = self.get_next_binding(0);
        Ok(ShaderCompilerOutput {
            vertex: vertex.0,
            fragment: fragment.0,
            context: NagaMslContext {
                fragment: NagaMslModule {
                    translation_info: fragment.1,
                    module: self.fragment,
                },
                vertex: NagaMslModule {
                    translation_info: vertex.1,
                    module: self.vertex,
                },
                next_free_binding: vertex_binding,
            },
        })
    }

    fn compile_boxed(
        self: Box<Self>,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, Self::Context>, ShaderCompileError> {
        <NagaReflect as CompileShader<MSL>>::compile(*self, options)
    }
}

#[cfg(test)]
mod test {
    use crate::back::targets::MSL;
    use crate::back::{CompileShader, FromCompilation};
    use crate::reflect::naga::{Naga, NagaLoweringOptions};
    use crate::reflect::semantics::{Semantic, ShaderSemantics, UniformSemantic, UniqueSemantics};
    use crate::reflect::ReflectShader;
    use bitflags::Flags;
    use librashader_common::map::{FastHashMap, ShortString};
    use librashader_preprocess::ShaderSource;
    use spirv_cross::msl;

    #[test]
    pub fn test_into() {
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

        let mut msl = <MSL as FromCompilation<_, Naga>>::from_compilation(compilation).unwrap();

        msl.reflect(
            0,
            &ShaderSemantics {
                uniform_semantics,
                texture_semantics: Default::default(),
            },
        )
        .expect("");

        let compiled = msl.compile(Some(msl::Version::V2_0)).unwrap();

        println!(
            "{:?}",
            compiled.context.fragment.translation_info.entry_point_names
        );
    }
}
