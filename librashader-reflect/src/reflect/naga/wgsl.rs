use crate::back::targets::WGSL;
use crate::back::wgsl::NagaWgslContext;
use crate::back::{CompileShader, ShaderCompilerOutput};
use crate::error::ShaderCompileError;
use crate::reflect::naga::{NagaLoweringOptions, NagaReflect};
use naga::back::wgsl::WriterFlags;
use naga::valid::{Capabilities, ValidationFlags};
use naga::Module;

impl CompileShader<WGSL> for NagaReflect {
    type Options = NagaLoweringOptions;
    type Context = NagaWgslContext;

    fn compile(
        mut self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<String, Self::Context>, ShaderCompileError> {
        fn write_wgsl(module: &Module) -> Result<String, ShaderCompileError> {
            let mut valid =
                naga::valid::Validator::new(ValidationFlags::all(), Capabilities::empty());
            let info = valid.validate(&module)?;

            let wgsl = naga::back::wgsl::write_string(&module, &info, WriterFlags::EXPLICIT_TYPES)?;
            Ok(wgsl)
        }

        self.do_lowering(&options);

        let fragment = write_wgsl(&self.fragment)?;
        let vertex = write_wgsl(&self.vertex)?;
        Ok(ShaderCompilerOutput {
            vertex,
            fragment,
            context: NagaWgslContext {
                fragment: self.fragment,
                vertex: self.vertex,
            },
        })
    }
}
