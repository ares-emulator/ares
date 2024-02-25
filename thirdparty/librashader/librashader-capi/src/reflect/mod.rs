use crate::error;

use librashader::presets::{ShaderPassConfig, ShaderPreset};
use librashader::reflect::semantics::ShaderSemantics;
use librashader::reflect::targets::SPIRV;
use librashader::reflect::{CompileShader, ReflectShader, ShaderCompilerOutput, ShaderReflection};
use librashader::{FilterMode, WrapMode};

use librashader::reflect::helper::image::{Image, UVDirection, RGBA8};
use librashader::reflect::SpirvCompilation;

pub(crate) struct LookupTexture {
    wrap_mode: WrapMode,
    /// The filter mode to use when sampling the texture.
    filter_mode: FilterMode,
    /// Whether or not to generate mipmaps for this texture.
    mipmap: bool,
    /// The image data of the texture
    image: Image,
}

pub(crate) struct PassReflection {
    reflection: ShaderReflection,
    config: ShaderPassConfig,
    spirv: ShaderCompilerOutput<Vec<u32>>,
}
pub(crate) struct FilterReflection {
    semantics: ShaderSemantics,
    passes: Vec<PassReflection>,
    textures: Vec<LookupTexture>,
}

impl FilterReflection {
    pub fn load_from_preset(
        preset: ShaderPreset,
        direction: UVDirection,
    ) -> Result<FilterReflection, error::LibrashaderError> {
        let (passes, textures) = (preset.shaders, preset.textures);

        let (passes, semantics) = librashader::reflect::helper::compile_preset_passes::<
            Glslang,
            SPIRV,
            SpirvCompilation,
            error::LibrashaderError,
        >(passes, &textures)?;

        let mut reflects = Vec::new();

        for (index, (config, _source, mut compiler)) in passes.into_iter().enumerate() {
            let reflection = compiler.reflect(index, &semantics)?;
            let words = compiler.compile(None)?;
            reflects.push(PassReflection {
                reflection,
                config,
                spirv: words,
            })
        }

        let textures = textures
            .into_iter()
            .map(|texture| {
                let lut = Image::<RGBA8>::load(&texture.path, direction)
                    .map_err(|e| error::LibrashaderError::UnknownError(Box::new(e)))?;
                Ok(LookupTexture {
                    wrap_mode: texture.wrap_mode,
                    filter_mode: texture.filter_mode,
                    mipmap: texture.mipmap,
                    image: lut,
                })
            })
            .collect::<Result<Vec<LookupTexture>, error::LibrashaderError>>()?;

        Ok(FilterReflection {
            semantics,
            passes: reflects,
            textures,
        })
    }
}
