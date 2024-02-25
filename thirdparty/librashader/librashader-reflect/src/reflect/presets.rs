use crate::back::targets::OutputTarget;
use crate::back::{CompilerBackend, FromCompilation};
use crate::error::{ShaderCompileError, ShaderReflectError};
use crate::front::{ShaderInputCompiler, ShaderReflectObject};
use crate::reflect::semantics::{
    Semantic, ShaderSemantics, TextureSemantics, UniformSemantic, UniqueSemantics,
};
use librashader_common::map::FastHashMap;
use librashader_preprocess::{PreprocessError, ShaderSource};
use librashader_presets::{ShaderPassConfig, TextureConfig};

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
pub type ShaderPassArtifact<T> = (ShaderPassConfig, ShaderSource, CompilerBackend<T>);

impl<T: OutputTarget> CompilePresetTarget for T {}

/// Trait for target shading languages that can compile output with
/// shader preset metdata.
pub trait CompilePresetTarget: OutputTarget {
    /// Compile passes of a shader preset given the applicable
    /// shader output target, compilation type, and resulting error.
    fn compile_preset_passes<I, R, E>(
        passes: Vec<ShaderPassConfig>,
        textures: &[TextureConfig],
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
fn compile_preset_passes<T, I, R, E>(
    passes: Vec<ShaderPassConfig>,
    textures: &[TextureConfig],
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
    let mut uniform_semantics: FastHashMap<String, UniformSemantic> = Default::default();
    let mut texture_semantics: FastHashMap<String, Semantic<TextureSemantics>> = Default::default();

    let passes = passes
        .into_iter()
        .map(|shader| {
            let source: ShaderSource = ShaderSource::load(&shader.name)?;

            let compiled = I::Compiler::compile(&source)?;
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
            Ok::<_, E>((shader, source, reflect))
        })
        .collect::<Result<Vec<(ShaderPassConfig, ShaderSource, CompilerBackend<_>)>, E>>()?;

    for details in &passes {
        insert_pass_semantics(&mut uniform_semantics, &mut texture_semantics, &details.0)
    }
    insert_lut_semantics(textures, &mut uniform_semantics, &mut texture_semantics);

    let semantics = ShaderSemantics {
        uniform_semantics,
        texture_semantics,
    };

    Ok((passes, semantics))
}

/// Insert the available semantics for the input pass config into the provided semantic maps.
fn insert_pass_semantics(
    uniform_semantics: &mut FastHashMap<String, UniformSemantic>,
    texture_semantics: &mut FastHashMap<String, Semantic<TextureSemantics>>,
    config: &ShaderPassConfig,
) {
    let Some(alias) = &config.alias else {
        return;
    };

    // Ignore empty aliases
    if alias.trim().is_empty() {
        return;
    }

    let index = config.id as usize;

    // PassOutput
    texture_semantics.insert(
        alias.clone(),
        Semantic {
            semantics: TextureSemantics::PassOutput,
            index,
        },
    );
    uniform_semantics.insert(
        format!("{alias}Size"),
        UniformSemantic::Texture(Semantic {
            semantics: TextureSemantics::PassOutput,
            index,
        }),
    );

    // PassFeedback
    texture_semantics.insert(
        format!("{alias}Feedback"),
        Semantic {
            semantics: TextureSemantics::PassFeedback,
            index,
        },
    );
    uniform_semantics.insert(
        format!("{alias}FeedbackSize"),
        UniformSemantic::Texture(Semantic {
            semantics: TextureSemantics::PassFeedback,
            index,
        }),
    );
}

/// Insert the available semantics for the input texture config into the provided semantic maps.
fn insert_lut_semantics(
    textures: &[TextureConfig],
    uniform_semantics: &mut FastHashMap<String, UniformSemantic>,
    texture_semantics: &mut FastHashMap<String, Semantic<TextureSemantics>>,
) {
    for (index, texture) in textures.iter().enumerate() {
        texture_semantics.insert(
            texture.name.clone(),
            Semantic {
                semantics: TextureSemantics::User,
                index,
            },
        );

        uniform_semantics.insert(
            format!("{}Size", texture.name),
            UniformSemantic::Texture(Semantic {
                semantics: TextureSemantics::User,
                index,
            }),
        );
    }
}
