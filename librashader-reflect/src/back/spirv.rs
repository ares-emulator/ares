use crate::back::targets::SPIRV;
use crate::back::{CompileShader, CompilerBackend, FromCompilation, ShaderCompilerOutput};
use crate::error::{ShaderCompileError, ShaderReflectError};
use crate::front::SpirvCompilation;
use crate::reflect::cross::glsl::GlslReflect;
use crate::reflect::cross::SpirvCross;
use crate::reflect::naga::{Naga, NagaLoweringOptions, NagaReflect};
use crate::reflect::semantics::ShaderSemantics;
use crate::reflect::{ReflectShader, ShaderReflection};
use naga::Module;

pub(crate) struct WriteSpirV {
    // rely on GLSL to provide out reflection but we don't actually need the AST.
    pub(crate) reflect: GlslReflect,
    pub(crate) vertex: Vec<u32>,
    pub(crate) fragment: Vec<u32>,
}

impl FromCompilation<SpirvCompilation, SpirvCross> for SPIRV {
    type Target = SPIRV;
    type Options = Option<()>;
    type Context = ();
    type Output = impl CompileShader<Self::Target, Options = Self::Options, Context = Self::Context>
        + ReflectShader;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        let reflect = GlslReflect::try_from(&compile)?;
        let vertex = compile.vertex;
        let fragment = compile.fragment;
        Ok(CompilerBackend {
            backend: WriteSpirV {
                reflect,
                vertex,
                fragment,
            },
        })
    }
}

impl ReflectShader for WriteSpirV {
    fn reflect(
        &mut self,
        pass_number: usize,
        semantics: &ShaderSemantics,
    ) -> Result<ShaderReflection, ShaderReflectError> {
        self.reflect.reflect(pass_number, semantics)
    }
}

impl CompileShader<SPIRV> for WriteSpirV {
    type Options = Option<()>;
    type Context = ();

    fn compile(
        self,
        _options: Self::Options,
    ) -> Result<ShaderCompilerOutput<Vec<u32>, Self::Context>, ShaderCompileError> {
        Ok(ShaderCompilerOutput {
            vertex: self.vertex,
            fragment: self.fragment,
            context: (),
        })
    }
}

/// The context for a SPIRV compilation via Naga
pub struct NagaSpirvContext {
    pub fragment: Module,
    pub vertex: Module,
}

impl FromCompilation<SpirvCompilation, Naga> for SPIRV {
    type Target = SPIRV;
    type Options = NagaSpirvOptions;
    type Context = NagaSpirvContext;
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

pub struct NagaSpirvOptions {
    pub lowering: NagaLoweringOptions,
    pub version: (u8, u8),
}
