use crate::back::targets::OutputTarget;
use crate::back::{CompilerBackend, FromCompilation};
use crate::error::{ShaderCompileError, ShaderReflectError};
use crate::front::{ShaderInputCompiler, ShaderReflectObject};
use crate::reflect::semantics::{
    Semantic, ShaderSemantics, TextureSemantics, UniformSemantic, UniqueSemantics,
};
use librashader_common::map::{FastHashMap, ShortString};
use librashader_pack::PassResource;
use librashader_preprocess::{PreprocessError, ShaderSource};
use librashader_presets::{ShaderPreset, TextureMeta};

/// Artifacts of a reflected and compiled shader pass.
///
/// The [`CompileReflectShader`](crate::back::CompileReflectShader) trait allows you to name
/// the type of compiler artifact returned by a pair of output shader target and compilation
/// instance as a TAIT like so.
///
/// ```rust
/// #![feature(type_alias_impl_trait)]
/// use librashader_reflect::back::CompileReflectShader;
/// use librashader_reflect::back::targets::{GLSL, SPIRV};
/// use librashader_reflect::front::SpirvCompilation;
/// use librashader_reflect::reflect::cross::SpirvCross;
/// use librashader_reflect::reflect::presets::ShaderPassArtifact;
///
/// type VulkanPassMeta = ShaderPassArtifact<impl CompileReflectShader<SPIRV, SpirvCompilation, SpirvCross>>;
/// ```
///
/// This allows a runtime to not name the backing type of the compiled artifact if not necessary.
pub type ShaderPassArtifact<T> = (PassResource, CompilerBackend<T>);

impl<T: OutputTarget> CompilePresetTarget for T {}

/// Trait for target shading languages that can compile output with
/// shader preset metdata.
pub trait CompilePresetTarget: OutputTarget {
    /// Compile passes of a shader preset given the applicable
    /// shader output target, compilation type, and resulting error.
    fn compile_preset_passes<'a, I, R, E>(
        passes: impl IntoIterator<Item = PassResource>,
        textures: impl Iterator<Item = &'a TextureMeta>,
    ) -> Result<
        (
            Vec<ShaderPassArtifact<<Self as FromCompilation<I, R>>::Output>>,
            ShaderSemantics,
        ),
        E,
    >
    where
        I: ShaderReflectObject,
        Self: Sized,
        Self: FromCompilation<I, R>,
        I::Compiler: ShaderInputCompiler<I>,
        E: From<PreprocessError>,
        E: From<ShaderReflectError>,
        E: From<ShaderCompileError>,
    {
        compile_preset_passes::<Self, I, R, E>(passes, textures)
    }
}

/// Compile passes of a shader preset given the applicable
/// shader output target, compilation type, and resulting error.
fn compile_preset_passes<'a, T, I, R, E>(
    passes: impl IntoIterator<Item = PassResource>,
    textures: impl Iterator<Item = &'a TextureMeta>,
) -> Result<
    (
        Vec<ShaderPassArtifact<<T as FromCompilation<I, R>>::Output>>,
        ShaderSemantics,
    ),
    E,
>
where
    I: ShaderReflectObject,
    T: OutputTarget,
    T: FromCompilation<I, R>,
    I::Compiler: ShaderInputCompiler<I>,
    E: From<PreprocessError>,
    E: From<ShaderReflectError>,
    E: From<ShaderCompileError>,
{
    let mut uniform_semantics: FastHashMap<ShortString, UniformSemantic> = Default::default();
    let mut texture_semantics: FastHashMap<ShortString, Semantic<TextureSemantics>> =
        Default::default();

    let artifacts = passes
        .into_iter()
        .map(|shader| {
            let source = &shader.data;
            let compiled = I::Compiler::compile(source)?;
            let reflect = T::from_compilation(compiled)?;

            for parameter in source.parameters.values() {
                uniform_semantics.insert(
                    parameter.id.clone(),
                    UniformSemantic::Unique(Semantic {
                        semantics: UniqueSemantics::FloatParameter,
                        index: (),
                    }),
                );
            }
            Ok::<_, E>((shader, reflect))
        })
        .collect::<Result<Vec<(PassResource, CompilerBackend<_>)>, E>>()?;

    for (pass, _) in artifacts.iter() {
        insert_pass_semantics(
            &mut uniform_semantics,
            &mut texture_semantics,
            pass.meta.alias.as_ref(),
            pass.meta.id as usize,
        );
        insert_pass_semantics(
            &mut uniform_semantics,
            &mut texture_semantics,
            pass.data.name.as_ref(),
            pass.meta.id as usize,
        );
    }

    insert_lut_semantics(textures, &mut uniform_semantics, &mut texture_semantics);

    let semantics = ShaderSemantics {
        uniform_semantics,
        texture_semantics,
    };

    Ok((artifacts, semantics))
}

/// Insert the available semantics for the input pass config into the provided semantic maps.
fn insert_pass_semantics(
    uniform_semantics: &mut FastHashMap<ShortString, UniformSemantic>,
    texture_semantics: &mut FastHashMap<ShortString, Semantic<TextureSemantics>>,
    alias: Option<&ShortString>,
    index: usize,
) {
    let Some(alias) = alias else {
        return;
    };

    // Ignore empty aliases
    if alias.trim().is_empty() {
        return;
    }

    // PassOutput
    texture_semantics.insert(
        alias.clone(),
        Semantic {
            semantics: TextureSemantics::PassOutput,
            index,
        },
    );

    let mut alias_size = alias.clone();
    alias_size.push_str("Size");
    uniform_semantics.insert(
        alias_size,
        UniformSemantic::Texture(Semantic {
            semantics: TextureSemantics::PassOutput,
            index,
        }),
    );

    let mut alias_feedback = alias.clone();
    alias_feedback.push_str("Feedback");
    // PassFeedback
    texture_semantics.insert(
        alias_feedback,
        Semantic {
            semantics: TextureSemantics::PassFeedback,
            index,
        },
    );

    let mut alias_feedback_size = alias.clone();
    alias_feedback_size.push_str("FeedbackSize");
    uniform_semantics.insert(
        alias_feedback_size,
        UniformSemantic::Texture(Semantic {
            semantics: TextureSemantics::PassFeedback,
            index,
        }),
    );
}

/// Insert the available semantics for the input texture config into the provided semantic maps.
fn insert_lut_semantics<'a>(
    textures: impl Iterator<Item = &'a TextureMeta>,
    uniform_semantics: &mut FastHashMap<ShortString, UniformSemantic>,
    texture_semantics: &mut FastHashMap<ShortString, Semantic<TextureSemantics>>,
) {
    for (index, texture) in textures.enumerate() {
        let mut size_semantic = texture.name.clone();
        size_semantic.push_str("Size");

        texture_semantics.insert(
            texture.name.clone(),
            Semantic {
                semantics: TextureSemantics::User,
                index,
            },
        );

        uniform_semantics.insert(
            size_semantic,
            UniformSemantic::Texture(Semantic {
                semantics: TextureSemantics::User,
                index,
            }),
        );
    }
}

impl ShaderSemantics {
    /// Create pass semantics for a single pass in the given shader preset.
    ///
    /// This is meant as a convenience function for reflection use only.
    pub fn create_pass_semantics<E>(
        preset: &ShaderPreset,
        index: usize,
    ) -> Result<ShaderSemantics, E>
    where
        E: From<ShaderReflectError>,
        E: From<PreprocessError>,
    {
        let mut uniform_semantics: FastHashMap<ShortString, UniformSemantic> = Default::default();
        let mut texture_semantics: FastHashMap<ShortString, Semantic<TextureSemantics>> =
            Default::default();

        let config = preset
            .passes
            .get(index)
            .ok_or_else(|| PreprocessError::InvalidStage)?;

        let source = ShaderSource::load(&config.path)?;

        for parameter in source.parameters.values() {
            uniform_semantics.insert(
                parameter.id.clone(),
                UniformSemantic::Unique(Semantic {
                    semantics: UniqueSemantics::FloatParameter,
                    index: (),
                }),
            );
        }

        insert_pass_semantics(
            &mut uniform_semantics,
            &mut texture_semantics,
            config.meta.alias.as_ref(),
            config.meta.id as usize,
        );
        insert_pass_semantics(
            &mut uniform_semantics,
            &mut texture_semantics,
            source.name.as_ref(),
            config.meta.id as usize,
        );
        insert_lut_semantics(
            preset.textures.iter().map(|t| &t.meta),
            &mut uniform_semantics,
            &mut texture_semantics,
        );

        Ok(ShaderSemantics {
            uniform_semantics,
            texture_semantics,
        })
    }
}
