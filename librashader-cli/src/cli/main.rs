use anyhow::anyhow;
use clap::{Parser, Subcommand};
use image::codecs::png::PngEncoder;
use librashader::presets::context::ContextItem;
use librashader::presets::{ShaderPreset, ShaderPresetPack, WildcardContext};
use librashader::reflect::cross::{GlslVersion, HlslShaderModel, MslVersion, SpirvCross};
use librashader::reflect::naga::{Naga, NagaLoweringOptions};
use librashader::reflect::semantics::ShaderSemantics;
use librashader::reflect::{CompileShader, FromCompilation, ReflectShader, SpirvCompilation};
use librashader::runtime::Size;
use librashader::{FastHashMap, ShortString};
use librashader_runtime::parameters::RuntimeParameters;
use librashader_test::render::{CommonFrameOptions, RenderTest};
use std::fs::File;
use std::io::Write;
use std::path::{Path, PathBuf};

/// Helpers and utilities to reflect and debug 'slang' shaders and presets.
#[derive(Parser, Debug)]
#[command(version, about)]
struct Args {
    #[command(subcommand)]
    command: Commands,
}

#[derive(clap::Args, Debug)]
struct PresetArgs {
    /// The path to the shader preset to load.
    #[arg(short, long)]
    preset: PathBuf,
    /// Additional wildcard options, comma separated with equals signs. The PRESET and PRESET_DIR
    /// wildcards are always added to the preset parsing context.
    ///
    /// For example, CONTENT-DIR=MyVerticalGames,GAME=mspacman
    #[arg(short, long, value_delimiter = ',', num_args = 1..)]
    wildcards: Option<Vec<String>>,
}

#[derive(clap::Args, Debug)]
struct RenderArgs {
    /// The frame to render.
    ///
    /// The renderer will run up to the number of frames specified here
    /// to ensure feedback and history.
    #[arg(short, long, default_value_t = 0)]
    frame: usize,
    /// The dimensions of the image.
    ///
    /// This is given in either explicit dimensions `WIDTHxHEIGHT`, or a
    /// percentage of the input image in `SCALE%`.
    #[arg(short, long)]
    dimensions: Option<String>,
    /// Parameters to pass to the shader preset, comma separated with equals signs.
    ///
    /// For example, crt_gamma=2.5,halation_weight=0.001
    #[arg(long, value_delimiter = ',', num_args = 1..)]
    params: Option<Vec<String>>,
    /// Set the number of passes enabled for the preset.
    #[arg(long)]
    passes_enabled: Option<usize>,
    /// The path to the input image.
    #[arg(short, long)]
    image: PathBuf,
    #[clap(flatten)]
    options: Option<FrameOptionsArgs>,
}

impl From<FrameOptionsArgs> for CommonFrameOptions {
    fn from(value: FrameOptionsArgs) -> Self {
        Self {
            clear_history: false,
            frame_direction: value.frame_direction,
            rotation: value.rotation,
            total_subframes: value.total_subframes,
            current_subframe: value.current_subframe,
        }
    }
}

#[derive(clap::Args, Debug)]
struct FrameOptionsArgs {
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    #[arg(long, default_value_t = 1, allow_hyphen_values = true)]
    pub frame_direction: i32,
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 3 = 270deg.
    #[arg(long, default_value_t = 0)]
    pub rotation: u32,
    /// The total number of subframes ran. Default is 1.
    #[arg(long, default_value_t = 1)]
    pub total_subframes: u32,
    /// The current sub frame. Default is 1.
    #[arg(long, default_value_t = 1)]
    pub current_subframe: u32,
}

#[derive(Subcommand, Debug)]
enum Commands {
    /// Render a shader preset against an image
    Render {
        #[clap(flatten)]
        preset: PresetArgs,
        #[clap(flatten)]
        render: RenderArgs,
        /// The path to the output image
        ///
        /// If `-`, writes the image in PNG format to stdout.
        #[arg(short, long)]
        out: PathBuf,
        /// The runtime to use to render the shader preset.
        #[arg(value_enum, short, long)]
        runtime: Runtime,
    },
    /// Compare two runtimes and get a similarity score between the two
    /// runtimes rendering the same frame
    Compare {
        #[clap(flatten)]
        preset: PresetArgs,
        #[clap(flatten)]
        render: RenderArgs,
        /// The runtime to compare against
        #[arg(value_enum, short, long)]
        left: Runtime,
        /// The runtime to compare to
        #[arg(value_enum, short, long)]
        right: Runtime,
        /// The path to write the similarity image.
        ///
        /// If `-`, writes the image to stdout.
        #[arg(short, long)]
        out: Option<PathBuf>,
    },
    /// Parse a preset and get a JSON representation of the data.
    Parse {
        #[clap(flatten)]
        preset: PresetArgs,
    },
    /// Create a serialized preset pack from a shader preset.
    Pack {
        #[clap(flatten)]
        preset: PresetArgs,
        /// The path to write the output
        ///
        /// If `-`, writes the output to stdout
        #[arg(short, long)]
        out: PathBuf,
        /// The file format to output.
        #[arg(value_enum, short, long)]
        format: PackFormat,
    },
    /// Get the raw GLSL output of a preprocessed shader.
    Preprocess {
        /// The path to the slang shader.
        #[arg(short, long)]
        shader: PathBuf,
        /// The item to output.
        ///
        /// `json` will print a JSON representation of the preprocessed shader.
        #[arg(value_enum, short, long)]
        output: PreprocessOutput,
    },
    /// Transpile a shader to the given format.
    Transpile {
        /// The path to the slang shader.
        #[arg(short, long)]
        shader: PathBuf,

        /// The shader stage to output
        #[arg(value_enum, short = 'o', long)]
        stage: TranspileStage,

        /// The output format.
        #[arg(value_enum, short, long)]
        format: TranspileFormat,

        /// The version of the output format to parse as, if applicable
        ///
        /// For GLSL, this should be an string corresponding to a GLSL version (e.g. '330', or '300es', or '300 es').
        ///
        /// For HLSL, this is a shader model version as an integer (50), or a version in the format MAJ_MIN (5_0), or MAJ.MIN (5.0).
        ///
        /// For MSL, this is the shader language version as an integer in format
        /// <MMmmpp>(30100), or a version in the format MAJ_MIN (3_1), or MAJ.MIN (3.1).
        ///
        /// For SPIR-V, if this is the string "raw-id", then shows raw ID values instead of friendly names.
        #[arg(short, long)]
        version: Option<String>,
    },
    /// Reflect the shader relative to a preset, giving information about semantics used in a slang shader.
    Reflect {
        #[clap(flatten)]
        preset: PresetArgs,

        /// The pass index to use.
        #[arg(short, long)]
        index: usize,

        #[arg(value_enum, short, long, default_value = "cross")]
        backend: ReflectionBackend,
    },
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum PreprocessOutput {
    #[clap(name = "fragment")]
    Fragment,
    #[clap(name = "vertex")]
    Vertex,
    #[clap(name = "params")]
    Params,
    #[clap(name = "passformat")]
    Format,
    #[clap(name = "json")]
    Json,
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum TranspileStage {
    #[clap(name = "fragment")]
    Fragment,
    #[clap(name = "vertex")]
    Vertex,
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum TranspileFormat {
    #[clap(name = "glsl")]
    GLSL,
    #[clap(name = "hlsl")]
    HLSL,
    #[clap(name = "wgsl")]
    WGSL,
    #[clap(name = "msl")]
    MSL,
    #[clap(name = "spirv")]
    SPIRV,
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum PackFormat {
    #[clap(name = "json")]
    JSON,
    #[clap(name = "msgpack")]
    MsgPack,
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum Runtime {
    #[cfg(feature = "opengl")]
    #[clap(name = "opengl3")]
    OpenGL3,
    #[cfg(feature = "opengl")]
    #[clap(name = "opengl4")]
    OpenGL4,
    #[cfg(feature = "vulkan")]
    #[clap(name = "vulkan")]
    Vulkan,
    #[cfg(feature = "wgpu")]
    #[clap(name = "wgpu")]
    Wgpu,
    #[cfg(all(windows, feature = "d3d9"))]
    #[clap(name = "d3d9")]
    Direct3D9,
    #[cfg(all(windows, feature = "d3d11"))]
    #[clap(name = "d3d11")]
    Direct3D11,
    #[cfg(all(windows, feature = "d3d12"))]
    #[clap(name = "d3d12")]
    Direct3D12,
    #[cfg(all(target_vendor = "apple", feature = "metal"))]
    #[clap(name = "metal")]
    Metal,
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum ReflectionBackend {
    #[clap(name = "cross")]
    SpirvCross,
    #[clap(name = "naga")]
    Naga,
}

macro_rules! get_runtime {
    ($rt:ident, $image:ident) => {
        match $rt {
            #[cfg(feature = "opengl")]
            Runtime::OpenGL3 => &mut librashader_test::render::gl::OpenGl3::new($image.as_path())?,
            #[cfg(feature = "opengl")]
            Runtime::OpenGL4 => &mut librashader_test::render::gl::OpenGl4::new($image.as_path())?,
            #[cfg(feature = "vulkan")]
            Runtime::Vulkan => &mut librashader_test::render::vk::Vulkan::new($image.as_path())?,
            #[cfg(feature = "wgpu")]
            Runtime::Wgpu => &mut librashader_test::render::wgpu::Wgpu::new($image.as_path())?,
            #[cfg(all(windows, feature = "d3d9"))]
            Runtime::Direct3D9 => {
                &mut librashader_test::render::d3d9::Direct3D9::new($image.as_path())?
            }
            #[cfg(all(windows, feature = "d3d11"))]
            Runtime::Direct3D11 => {
                &mut librashader_test::render::d3d11::Direct3D11::new($image.as_path())?
            }
            #[cfg(all(windows, feature = "d3d12"))]
            Runtime::Direct3D12 => {
                &mut librashader_test::render::d3d12::Direct3D12::new($image.as_path())?
            }
            #[cfg(all(target_vendor = "apple", feature = "metal"))]
            Runtime::Metal => &mut librashader_test::render::mtl::Metal::new($image.as_path())?,
        }
    };
}
pub fn main() -> Result<(), anyhow::Error> {
    let args = Args::parse();

    match args.command {
        Commands::Render {
            preset,
            render,
            out,
            runtime,
        } => {
            let PresetArgs { preset, wildcards } = preset;
            let RenderArgs {
                frame,
                dimensions,
                params,
                passes_enabled,
                image,
                options,
            } = render;

            let test: &mut dyn RenderTest = get_runtime!(runtime, image);
            let dimensions = parse_dimension(dimensions, test.image_size())?;
            let preset = get_shader_preset(preset, wildcards)?;
            let params = parse_params(params)?;

            let image = test.render_with_preset_and_params(
                preset,
                frame,
                Some(dimensions),
                Some(&|rp| set_params(rp, &params, passes_enabled)),
                options.map(CommonFrameOptions::from),
            )?;

            if out.as_path() == Path::new("-") {
                let out = std::io::stdout();
                image.write_with_encoder(PngEncoder::new(out))?;
            } else {
                image.save(out)?;
            }
        }
        Commands::Compare {
            preset,
            render,
            left,
            right,
            out,
        } => {
            let PresetArgs { preset, wildcards } = preset;
            let RenderArgs {
                frame,
                dimensions,
                params,
                passes_enabled,
                image,
                options,
            } = render;

            let left: &mut dyn RenderTest = get_runtime!(left, image);
            let right: &mut dyn RenderTest = get_runtime!(right, image);

            let dimensions = parse_dimension(dimensions, left.image_size())?;
            let params = parse_params(params)?;

            let left_preset = get_shader_preset(preset.clone(), wildcards.clone())?;
            let left_image = left.render_with_preset_and_params(
                left_preset,
                frame,
                Some(dimensions),
                Some(&|rp| set_params(rp, &params, passes_enabled)),
                None,
            )?;

            let right_preset = get_shader_preset(preset.clone(), wildcards.clone())?;
            let right_image = right.render_with_preset_and_params(
                right_preset,
                frame,
                Some(dimensions),
                Some(&|rp| set_params(rp, &params, passes_enabled)),
                options.map(CommonFrameOptions::from),
            )?;

            let similarity = image_compare::rgba_hybrid_compare(&left_image, &right_image)?;
            print!("{}", similarity.score);

            if let Some(out) = out {
                let image = similarity.image.to_color_map();
                if out.as_path() == Path::new("-") {
                    let out = std::io::stdout();
                    image.write_with_encoder(PngEncoder::new(out))?;
                } else {
                    image.save(out)?;
                }
            }
        }
        Commands::Parse { preset } => {
            let PresetArgs { preset, wildcards } = preset;

            let preset = get_shader_preset(preset, wildcards)?;
            let out = serde_json::to_string_pretty(&preset)?;
            print!("{out:}");
        }
        Commands::Preprocess { shader, output } => {
            let source = librashader::preprocess::ShaderSource::load(shader.as_path())?;
            match output {
                PreprocessOutput::Fragment => print!("{}", source.fragment),
                PreprocessOutput::Vertex => print!("{}", source.vertex),
                PreprocessOutput::Params => {
                    print!("{}", serde_json::to_string_pretty(&source.parameters)?)
                }
                PreprocessOutput::Format => print!("{:?}", source.format),
                PreprocessOutput::Json => print!("{}", serde_json::to_string_pretty(&source)?),
            }
        }
        Commands::Transpile {
            shader,
            stage,
            format,
            version,
        } => {
            let source = librashader::preprocess::ShaderSource::load(shader.as_path())?;
            let compilation = SpirvCompilation::try_from(&source)?;
            let output = match format {
                TranspileFormat::GLSL => {
                    let mut compilation =
                        librashader::reflect::targets::GLSL::from_compilation(compilation)?;
                    compilation.validate()?;

                    let version = version
                        .map(|s| parse_glsl_version(&s))
                        .unwrap_or(Ok(GlslVersion::Glsl330))?;

                    let output = compilation.compile(version)?;
                    TranspileOutput {
                        vertex: output.vertex,
                        fragment: output.fragment,
                    }
                }
                TranspileFormat::HLSL => {
                    let mut compilation =
                        librashader::reflect::targets::HLSL::from_compilation(compilation)?;
                    compilation.validate()?;

                    let shader_model = version
                        .map(|s| parse_hlsl_version(&s))
                        .unwrap_or(Ok(HlslShaderModel::ShaderModel5_0))?;

                    let output = compilation.compile(Some(shader_model))?;
                    TranspileOutput {
                        vertex: output.vertex,
                        fragment: output.fragment,
                    }
                }
                TranspileFormat::WGSL => {
                    let mut compilation =
                        librashader::reflect::targets::WGSL::from_compilation(compilation)?;
                    compilation.validate()?;
                    let output = compilation.compile(NagaLoweringOptions {
                        write_pcb_as_ubo: true,
                        sampler_bind_group: 1,
                    })?;
                    TranspileOutput {
                        vertex: output.vertex,
                        fragment: output.fragment,
                    }
                }
                TranspileFormat::MSL => {
                    let mut compilation =
                        <librashader::reflect::targets::MSL as FromCompilation<
                            SpirvCompilation,
                            SpirvCross,
                        >>::from_compilation(compilation)?;
                    compilation.validate()?;

                    let version = version
                        .map(|s| parse_msl_version(&s))
                        .unwrap_or(Ok(MslVersion::new(1, 2, 0)))?;

                    let output = compilation.compile(Some(version))?;

                    TranspileOutput {
                        vertex: output.vertex,
                        fragment: output.fragment,
                    }
                }
                TranspileFormat::SPIRV => {
                    let mut compilation =
                        <librashader::reflect::targets::SPIRV as FromCompilation<
                            SpirvCompilation,
                            SpirvCross,
                        >>::from_compilation(compilation)?;
                    compilation.validate()?;
                    let output = compilation.compile(None)?;

                    let raw = version.is_some_and(|s| s == "raw-id");
                    TranspileOutput {
                        vertex: spirv_to_dis(output.vertex, raw)?,
                        fragment: spirv_to_dis(output.fragment, raw)?,
                    }
                }
            };

            let print = match stage {
                TranspileStage::Fragment => output.fragment,
                TranspileStage::Vertex => output.vertex,
            };

            print!("{print}")
        }
        Commands::Reflect {
            preset,
            index,
            backend,
        } => {
            let PresetArgs { preset, wildcards } = preset;

            let preset = get_shader_preset(preset, wildcards)?;
            let Some(shader) = preset.passes.get(index) else {
                return Err(anyhow!("Invalid pass index for the preset"));
            };

            let source = librashader::preprocess::ShaderSource::load(shader.path.as_path())?;
            let compilation = SpirvCompilation::try_from(&source)?;

            let semantics =
                ShaderSemantics::create_pass_semantics::<anyhow::Error>(&preset, index)?;

            let reflection = match backend {
                ReflectionBackend::SpirvCross => {
                    let mut compilation =
                        <librashader::reflect::targets::SPIRV as FromCompilation<
                            SpirvCompilation,
                            SpirvCross,
                        >>::from_compilation(compilation)?;
                    compilation.reflect(index, &semantics)?
                }
                ReflectionBackend::Naga => {
                    let mut compilation =
                        <librashader::reflect::targets::SPIRV as FromCompilation<
                            SpirvCompilation,
                            Naga,
                        >>::from_compilation(compilation)?;
                    compilation.reflect(index, &semantics)?
                }
            };

            print!("{}", serde_json::to_string_pretty(&reflection)?);
        }
        Commands::Pack {
            preset,
            out,
            format,
        } => {
            let PresetArgs { preset, wildcards } = preset;
            let preset = get_shader_preset(preset, wildcards)?;
            let preset = ShaderPresetPack::load_from_preset::<anyhow::Error>(preset)?;
            let output_bytes = match format {
                PackFormat::JSON => serde_json::to_vec_pretty(&preset)?,
                PackFormat::MsgPack => rmp_serde::to_vec(&preset)?,
            };

            if out.as_path() == Path::new("-") {
                let mut out = std::io::stdout();
                out.write_all(output_bytes.as_slice())?;
            } else {
                let mut file = File::create(out.as_path())?;
                file.write_all(output_bytes.as_slice())?;
            }
        }
    }

    Ok(())
}

struct TranspileOutput {
    vertex: String,
    fragment: String,
}

fn get_shader_preset(
    preset: PathBuf,
    wildcards: Option<Vec<String>>,
) -> anyhow::Result<ShaderPreset> {
    let mut context = WildcardContext::new();
    context.add_path_defaults(preset.as_path());
    if let Some(wildcards) = wildcards {
        for string in wildcards {
            let Some((left, right)) = string.split_once("=") else {
                return Err(anyhow!("Encountered invalid context string {string}"));
            };

            context.append_item(ContextItem::ExternContext(
                left.to_string(),
                right.to_string(),
            ))
        }
    }
    let preset = ShaderPreset::try_parse_with_context(preset, context)?;
    Ok(preset)
}

fn parse_params(
    assignments: Option<Vec<String>>,
) -> anyhow::Result<Option<FastHashMap<ShortString, f32>>> {
    let Some(assignments) = assignments else {
        return Ok(None);
    };

    let mut map = FastHashMap::default();
    for string in assignments {
        let Some((left, right)) = string.split_once("=") else {
            return Err(anyhow!("Encountered invalid parameter string {string}"));
        };

        let value = right
            .parse::<f32>()
            .map_err(|_| anyhow!("Encountered invalid parameter value: {right}"))?;

        map.insert(ShortString::from(left), value);
    }

    Ok(Some(map))
}

fn set_params(
    params: &RuntimeParameters,
    assignments: &Option<FastHashMap<ShortString, f32>>,
    passes_enabled: Option<usize>,
) {
    if let Some(passes_enabled) = passes_enabled {
        params.set_passes_enabled(passes_enabled)
    };

    let Some(assignments) = assignments else {
        return;
    };

    params.update_parameters(|params| {
        for (key, param) in assignments {
            params.insert(key.clone(), *param);
        }
    });
}

fn spirv_to_dis(spirv: Vec<u32>, raw: bool) -> anyhow::Result<String> {
    let binary = spq_spvasm::SpirvBinary::from(spirv);
    spq_spvasm::Disassembler::new()
        .print_header(true)
        .name_ids(!raw)
        .name_type_ids(!raw)
        .name_const_ids(!raw)
        .indent(true)
        .disassemble(&binary)
}

fn parse_glsl_version(version_str: &str) -> anyhow::Result<GlslVersion> {
    if version_str.contains("es") {
        let Some(version) = version_str.strip_suffix("es").map(|s| s.trim()) else {
            return Err(anyhow!("Unknown GLSL version"));
        };

        Ok(match version {
            "100" => GlslVersion::Glsl100Es,
            "300" => GlslVersion::Glsl300Es,
            "310" => GlslVersion::Glsl310Es,
            "320" => GlslVersion::Glsl320Es,
            _ => return Err(anyhow!("Unknown GLSL version")),
        })
    } else {
        Ok(match version_str {
            "100" => GlslVersion::Glsl100Es,
            "110" => GlslVersion::Glsl110,
            "120" => GlslVersion::Glsl120,
            "130" => GlslVersion::Glsl130,
            "140" => GlslVersion::Glsl140,
            "150" => GlslVersion::Glsl150,
            "300" => GlslVersion::Glsl300Es,
            "330" => GlslVersion::Glsl330,
            "310" => GlslVersion::Glsl310Es,
            "320" => GlslVersion::Glsl320Es,
            "400" => GlslVersion::Glsl400,
            "410" => GlslVersion::Glsl410,
            "420" => GlslVersion::Glsl420,
            "430" => GlslVersion::Glsl430,
            "440" => GlslVersion::Glsl440,
            "450" => GlslVersion::Glsl450,
            "460" => GlslVersion::Glsl460,
            _ => return Err(anyhow!("Unknown GLSL version")),
        })
    }
}

fn version_to_usize(version_str: &str) -> anyhow::Result<usize> {
    let version: &str = if version_str.contains("_") {
        &version_str.replace("_", "")
    } else if version_str.contains(".") {
        &version_str.replace(".", "")
    } else {
        version_str
    };

    let version = version
        .parse::<usize>()
        .map_err(|_| anyhow!("Invalid version string"))?;
    Ok(version)
}

fn parse_hlsl_version(version_str: &str) -> anyhow::Result<HlslShaderModel> {
    let version = version_to_usize(version_str)?;
    Ok(match version {
        30 => HlslShaderModel::ShaderModel3_0,
        40 => HlslShaderModel::ShaderModel4_0,
        50 => HlslShaderModel::ShaderModel5_0,
        51 => HlslShaderModel::ShaderModel5_1,
        60 => HlslShaderModel::ShaderModel6_0,
        61 => HlslShaderModel::ShaderModel6_1,
        62 => HlslShaderModel::ShaderModel6_2,
        63 => HlslShaderModel::ShaderModel6_3,
        64 => HlslShaderModel::ShaderModel6_4,
        65 => HlslShaderModel::ShaderModel6_5,
        66 => HlslShaderModel::ShaderModel6_6,
        67 => HlslShaderModel::ShaderModel6_7,
        68 => HlslShaderModel::ShaderModel6_8,
        _ => return Err(anyhow!("Unknown Shader Model")),
    })
}

fn parse_msl_version(version_str: &str) -> anyhow::Result<MslVersion> {
    let version = version_to_usize(version_str)?;
    Ok(match version {
        10 => MslVersion::new(1, 0, 0),
        11 => MslVersion::new(1, 1, 0),
        12 => MslVersion::new(1, 2, 0),
        20 => MslVersion::new(2, 0, 0),
        21 => MslVersion::new(2, 1, 0),
        22 => MslVersion::new(2, 2, 0),
        23 => MslVersion::new(2, 3, 0),
        24 => MslVersion::new(2, 4, 0),
        30 => MslVersion::new(3, 0, 0),
        31 => MslVersion::new(3, 1, 0),
        32 => MslVersion::new(3, 2, 0),
        n if n >= 10000 => {
            let major = n / 10000;
            let minor = (n - (major * 10000)) / 100;
            let patch = n - ((major * 10000) + (minor * 100));
            MslVersion::new(major as u32, minor as u32, patch as u32)
        }
        _ => return Err(anyhow!("Unknown MSL version")),
    })
}

fn parse_dimension(dimstr: Option<String>, image_dim: Size<u32>) -> anyhow::Result<Size<u32>> {
    let Some(dimstr) = dimstr else {
        return Ok(image_dim);
    };

    if dimstr.contains("x") {
        if let Some((Ok(width), Ok(height))) = dimstr
            .split_once("x")
            .map(|(width, height)| (width.parse::<u32>(), height.parse::<u32>()))
        {
            if width < 1 || height < 1 {
                return Err(anyhow!("Dimensions must be larger than 1x1"));
            }

            if width > 16384 || height > 16384 {
                return Err(anyhow!("Dimensions must not be larger than 16384x16384"));
            }
            return Ok(Size::new(width, height));
        }
    }

    if dimstr.ends_with("%") && dimstr.len() > 1 {
        if let Ok(percent) = dimstr.trim_end_matches("%").parse::<u32>() {
            let percent = percent as f32 / 100f32;
            let width = (image_dim.width as f32 * percent) as u32;
            let height = (image_dim.height as f32 * percent) as u32;

            if width < 1 || height < 1 {
                return Err(anyhow!("Dimensions must be larger than 1x1"));
            }

            if width > 16384 || height > 16384 {
                return Err(anyhow!("Dimensions must not be larger than 16384x16384"));
            }

            return Ok(Size { width, height });
        }
    }

    Err(anyhow!(
        "Invalid dimension syntax, must either in form WIDTHxHEIGHT or SCALE%"
    ))
}
