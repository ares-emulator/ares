#[cfg(all(target_os = "windows", feature = "dxil"))]
pub mod dxil;
pub mod glsl;
pub mod hlsl;
pub mod msl;
pub mod spirv;
pub mod targets;
pub mod wgsl;

use crate::back::targets::OutputTarget;
use crate::error::{ShaderCompileError, ShaderReflectError};
use crate::reflect::semantics::ShaderSemantics;
use crate::reflect::{ReflectShader, ShaderReflection};
use std::fmt::Debug;

/// The output of the shader compiler.
#[derive(Debug)]
pub struct ShaderCompilerOutput<T, Context = ()> {
    /// The output for the vertex shader.
    pub vertex: T,
    /// The output for the fragment shader.
    pub fragment: T,
    /// Additional context provided by the shader compiler.
    pub context: Context,
}

/// A trait for objects that can be compiled into a shader.
pub trait CompileShader<T: OutputTarget> {
    /// Options provided to the compiler.
    type Options;
    /// Additional context returned by the compiler after compilation.
    type Context;

    /// Consume the object and return the compiled output of the shader.
    fn compile(
        self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<T::Output, Self::Context>, ShaderCompileError>;
}

/// Marker trait for combinations of targets and compilations that can be reflected and compiled
/// successfully.
///
/// This trait is automatically implemented for reflected outputs that have [`FromCompilation`](crate::back::FromCompilation) implement
/// for a given target that also implement [`CompileShader`](crate::back::CompileShader) for that target.
pub trait CompileReflectShader<T: OutputTarget, C, S>:
    CompileShader<
        T,
        Options = <T as FromCompilation<C, S>>::Options,
        Context = <T as FromCompilation<C, S>>::Context,
    > + ReflectShader
where
    T: FromCompilation<C, S>,
{
}

impl<T, C, O, S> CompileReflectShader<T, C, S> for O
where
    T: OutputTarget,
    T: FromCompilation<C, S>,
    O: ReflectShader,
    O: CompileShader<
        T,
        Options = <T as FromCompilation<C, S>>::Options,
        Context = <T as FromCompilation<C, S>>::Context,
    >,
{
}

impl<T, E> CompileShader<E> for CompilerBackend<T>
where
    T: CompileShader<E>,
    E: OutputTarget,
{
    type Options = T::Options;
    type Context = T::Context;

    fn compile(
        self,
        options: Self::Options,
    ) -> Result<ShaderCompilerOutput<E::Output, Self::Context>, ShaderCompileError> {
        self.backend.compile(options)
    }
}

/// A trait for reflectable compilations that can be transformed
/// into an object ready for reflection or compilation.
///
/// `T` is the compiled reflectable form of the shader.
/// `S` is the semantics under which the shader is reflected.
///
/// librashader currently supports two semantics, [`SpirvCross`](crate::reflect::cross::SpirvCross)
pub trait FromCompilation<T, S> {
    /// The target that the transformed object is expected to compile for.
    type Target: OutputTarget;
    /// Options provided to the compiler.
    type Options;
    /// Additional context returned by the compiler after compilation.
    type Context;

    /// The output type after conversion.
    type Output: CompileShader<Self::Target, Context = Self::Context, Options = Self::Options>
        + ReflectShader;

    /// Tries to convert the input object into an object ready for compilation.
    fn from_compilation(compile: T) -> Result<CompilerBackend<Self::Output>, ShaderReflectError>;
}

/// A wrapper for a compiler backend.
pub struct CompilerBackend<T> {
    pub(crate) backend: T,
}

impl<T> ReflectShader for CompilerBackend<T>
where
    T: ReflectShader,
{
    fn reflect(
        &mut self,
        pass_number: usize,
        semantics: &ShaderSemantics,
    ) -> Result<ShaderReflection, ShaderReflectError> {
        self.backend.reflect(pass_number, semantics)
    }
}

#[cfg(test)]
mod test {
    use crate::front::{Glslang, ShaderInputCompiler};
    use librashader_preprocess::ShaderSource;

    pub fn test() {
        let result = ShaderSource::load("../test/basic.slang").unwrap();
        let _cross = Glslang::compile(&result).unwrap();
    }
}
