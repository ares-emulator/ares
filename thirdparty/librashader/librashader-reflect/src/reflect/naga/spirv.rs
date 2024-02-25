use crate::back::spirv::{NagaSpirvContext, NagaSpirvOptions};
use crate::back::targets::SPIRV;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::naga::NagaReflect;
use naga::back::spv::PipelineOptions;
use naga::valid::{Capabilities, ValidationFlags};
use naga::Module;

impl CompileShader<SPIRV> for NagaReflect {
    type Options = NagaSpirvOptions;
    type Context = NagaSpirvContext;

    fn compile(
        mut self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<Vec<u32>, Self::Context>, ShaderCompileError> {
        fn write_spv(
            module: &Module,
            stage: naga::ShaderStage,
            version: (u8, u8),
        ) -> Result<Vec<u32>, ShaderCompileError> {
            let mut valid =
                naga::valid::Validator::new(ValidationFlags::all(), Capabilities::empty());
            let info = valid.validate(&module)?;
            let mut options = naga::back::spv::Options::default();
            options.lang_version = version;

            let spv = naga::back::spv::write_vec(
                &module,
                &info,
                &options,
                Some(&PipelineOptions {
                    shader_stage: stage,
                    entry_point: "main".to_string(),
                }),
            )?;
            Ok(spv)
        }

        self.do_lowering(&options.lowering);

        let fragment = write_spv(&self.fragment, naga::ShaderStage::Fragment, options.version)?;
        let vertex = write_spv(&self.vertex, naga::ShaderStage::Vertex, options.version)?;
        Ok(ShaderCompilerOutput {
            vertex,
            fragment,
            context: NagaSpirvContext {
                fragment: self.fragment,
                vertex: self.vertex,
            },
        })
    }
}
