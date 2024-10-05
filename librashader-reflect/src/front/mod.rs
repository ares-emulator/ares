use crate::error::ShaderCompileError;
use librashader_preprocess::ShaderSource;
pub(crate) mod spirv_passes;

mod glslang;

/// The output of a shader compiler that is reflectable.
pub trait ShaderReflectObject: Sized {
    /// The compiler that produces this reflect object.
    type Compiler;
}

pub use crate::front::glslang::Glslang;

/// Trait for types that can compile shader sources into a compilation unit.
pub trait ShaderInputCompiler<O: ShaderReflectObject>: Sized {
    /// Compile the input shader source file into a compilation unit.
    fn compile(source: &ShaderSource) -> Result<O, ShaderCompileError>;
}

/// Marker trait for types that are the reflectable outputs of a shader compilation.
impl ShaderReflectObject for SpirvCompilation {
    type Compiler = Glslang;
}

/// A reflectable shader compilation via glslang.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct SpirvCompilation {
    pub(crate) vertex: Vec<u32>,
    pub(crate) fragment: Vec<u32>,
}

impl TryFrom<&ShaderSource> for SpirvCompilation {
    type Error = ShaderCompileError;

    /// Tries to compile SPIR-V from the provided shader source.
    fn try_from(source: &ShaderSource) -> Result<Self, Self::Error> {
        Glslang::compile(source)
    }
}
