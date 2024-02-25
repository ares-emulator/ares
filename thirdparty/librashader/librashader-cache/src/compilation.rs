//!  Cache helpers for `ShaderCompilation` objects to cache compiled SPIRV.
use librashader_preprocess::ShaderSource;
#[cfg(all(target_os = "windows", feature = "d3d"))]
use librashader_reflect::back::targets::DXIL;
use librashader_reflect::back::targets::{GLSL, HLSL, SPIRV};

use librashader_reflect::back::{CompilerBackend, FromCompilation};
use librashader_reflect::error::{ShaderCompileError, ShaderReflectError};
use librashader_reflect::front::{
    Glslang, ShaderInputCompiler, ShaderReflectObject, SpirvCompilation,
};

pub struct CachedCompilation<T> {
    compilation: T,
}

impl<T: ShaderReflectObject> ShaderReflectObject for CachedCompilation<T> {
    type Compiler = T::Compiler;
}

impl<T: ShaderReflectObject + for<'de> serde::Deserialize<'de> + serde::Serialize + Clone>
    ShaderInputCompiler<CachedCompilation<T>> for Glslang
where
    Glslang: ShaderInputCompiler<T>,
{
    fn compile(source: &ShaderSource) -> Result<CachedCompilation<T>, ShaderCompileError> {
        let cache = crate::cache::internal::get_cache();

        let Ok(cache) = cache else {
            return Ok(CachedCompilation {
                compilation: Glslang::compile(source)?,
            });
        };

        let key = {
            let mut hasher = blake3::Hasher::new();
            hasher.update(source.vertex.as_bytes());
            hasher.update(source.fragment.as_bytes());
            let hash = hasher.finalize();
            hash
        };

        let compilation = 'cached: {
            if let Ok(Some(cached)) =
                crate::cache::internal::get_blob(&cache, "spirv", key.as_bytes())
            {
                let decoded =
                    bincode::serde::decode_from_slice(&cached, bincode::config::standard())
                        .map(|(compilation, _)| CachedCompilation { compilation })
                        .ok();

                if let Some(compilation) = decoded {
                    break 'cached compilation;
                }
            }

            CachedCompilation {
                compilation: Glslang::compile(source)?,
            }
        };

        if let Ok(updated) =
            bincode::serde::encode_to_vec(&compilation.compilation, bincode::config::standard())
        {
            let Ok(()) =
                crate::cache::internal::set_blob(&cache, "spirv", key.as_bytes(), &updated)
            else {
                return Ok(compilation);
            };
        }

        Ok(compilation)
    }
}

#[cfg(all(target_os = "windows", feature = "d3d"))]
impl<T> FromCompilation<CachedCompilation<SpirvCompilation>, T> for DXIL
where
    DXIL: FromCompilation<SpirvCompilation, T>,
{
    type Target = <DXIL as FromCompilation<SpirvCompilation, T>>::Target;
    type Options = <DXIL as FromCompilation<SpirvCompilation, T>>::Options;
    type Context = <DXIL as FromCompilation<SpirvCompilation, T>>::Context;
    type Output = <DXIL as FromCompilation<SpirvCompilation, T>>::Output;

    fn from_compilation(
        compile: CachedCompilation<SpirvCompilation>,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        DXIL::from_compilation(compile.compilation)
    }
}

impl<T> FromCompilation<CachedCompilation<SpirvCompilation>, T> for HLSL
where
    HLSL: FromCompilation<SpirvCompilation, T>,
{
    type Target = <HLSL as FromCompilation<SpirvCompilation, T>>::Target;
    type Options = <HLSL as FromCompilation<SpirvCompilation, T>>::Options;
    type Context = <HLSL as FromCompilation<SpirvCompilation, T>>::Context;
    type Output = <HLSL as FromCompilation<SpirvCompilation, T>>::Output;

    fn from_compilation(
        compile: CachedCompilation<SpirvCompilation>,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        HLSL::from_compilation(compile.compilation)
    }
}

impl<T> FromCompilation<CachedCompilation<SpirvCompilation>, T> for GLSL
where
    GLSL: FromCompilation<SpirvCompilation, T>,
{
    type Target = <GLSL as FromCompilation<SpirvCompilation, T>>::Target;
    type Options = <GLSL as FromCompilation<SpirvCompilation, T>>::Options;
    type Context = <GLSL as FromCompilation<SpirvCompilation, T>>::Context;
    type Output = <GLSL as FromCompilation<SpirvCompilation, T>>::Output;

    fn from_compilation(
        compile: CachedCompilation<SpirvCompilation>,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        GLSL::from_compilation(compile.compilation)
    }
}

impl<T> FromCompilation<CachedCompilation<SpirvCompilation>, T> for SPIRV
where
    SPIRV: FromCompilation<SpirvCompilation, T>,
{
    type Target = <SPIRV as FromCompilation<SpirvCompilation, T>>::Target;
    type Options = <SPIRV as FromCompilation<SpirvCompilation, T>>::Options;
    type Context = <SPIRV as FromCompilation<SpirvCompilation, T>>::Context;
    type Output = <SPIRV as FromCompilation<SpirvCompilation, T>>::Output;

    fn from_compilation(
        compile: CachedCompilation<SpirvCompilation>,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        SPIRV::from_compilation(compile.compilation)
    }
}
