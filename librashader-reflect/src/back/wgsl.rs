use crate::back::targets::WGSL;
use crate::back::{CompileShader, CompilerBackend, FromCompilation};
use crate::error::ShaderReflectError;
use crate::front::SpirvCompilation;
use crate::reflect::naga::{Naga, NagaLoweringOptions, NagaReflect};
use crate::reflect::ReflectShader;
use naga::Module;

/// The context for a WGSL compilation via Naga
pub struct NagaWgslContext {
    pub fragment: Module,
    pub vertex: Module,
}

impl FromCompilation<SpirvCompilation, Naga> for WGSL {
    type Target = WGSL;
    type Options = NagaLoweringOptions;
    type Context = NagaWgslContext;
    type Output = impl CompileShader<Self::Target, Options = Self::Options, Context = Self::Context>
        + ReflectShader;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        Ok(CompilerBackend {
            backend: NagaReflect::try_from(&compile)?,
        })
    }
}

#[cfg(test)]
mod test {
    use crate::back::targets::WGSL;
    use crate::back::{CompileShader, FromCompilation};
    use crate::reflect::naga::NagaLoweringOptions;
    use crate::reflect::semantics::{Semantic, ShaderSemantics, UniformSemantic, UniqueSemantics};
    use crate::reflect::ReflectShader;
    use bitflags::Flags;
    use librashader_common::map::FastHashMap;
    use librashader_preprocess::ShaderSource;

    #[test]
    pub fn test_into() {
        // let result = ShaderSource::load("../test/shaders_slang/crt/shaders/crt-royale/src/crt-royale-scanlines-horizontal-apply-mask.slang").unwrap();
        // let result = ShaderSource::load("../test/shaders_slang/crt/shaders/crt-royale/src/crt-royale-scanlines-horizontal-apply-mask.slang").unwrap();
        let result = ShaderSource::load("../test/basic.slang").unwrap();

        let mut uniform_semantics: FastHashMap<String, UniformSemantic> = Default::default();

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

        let mut wgsl = WGSL::from_compilation(compilation).unwrap();

        wgsl.reflect(
            0,
            &ShaderSemantics {
                uniform_semantics,
                texture_semantics: Default::default(),
            },
        )
        .expect("");

        let compiled = wgsl
            .compile(NagaLoweringOptions {
                write_pcb_as_ubo: true,
                sampler_bind_group: 1,
            })
            .unwrap();

        println!("{}", compiled.fragment);

        // println!("{}", compiled.fragment);
        // let mut loader = rspirv::dr::Loader::new();
        // rspirv::binary::parse_words(compilation.vertex.as_binary(), &mut loader).unwrap();
        // let module = loader.module();
        //
        // let outputs: Vec<&Instruction> = module
        //     .types_global_values
        //     .iter()
        //     .filter(|i| i.class.opcode == Op::Variable)
        //     .collect();
        //
        // println!("{outputs:#?}");
    }
}
