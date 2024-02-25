use glob::glob;
use librashader::preprocess::ShaderSource;
use librashader::presets::ShaderPreset;
use rayon::prelude::*;
use std::error::Error;
use std::path::PathBuf;
use std::sync::RwLock;

use librashader::reflect::cross::SpirvCross;
use librashader::reflect::naga::Naga;
use librashader::reflect::targets::*;
use librashader::reflect::CompilePresetTarget;
use librashader::reflect::FromCompilation;
use librashader::reflect::OutputTarget;
use librashader::reflect::SpirvCompilation;

use once_cell::sync::Lazy;
static ALL_SLANG_PRESETS: Lazy<RwLock<Vec<(PathBuf, ShaderPreset)>>> =
    Lazy::new(|| RwLock::new(collect_all_loadable_slang_presets()));

fn collect_all_slang_presets(collect_is_error: bool) -> Vec<(PathBuf, ShaderPreset)> {
    let presets = glob("../test/shaders_slang/**/*.slangp")
        .unwrap()
        .collect::<Vec<_>>()
        .into_par_iter()
        .filter_map(|entry| {
            if let Ok(path) = entry {
                match ShaderPreset::try_parse(&path) {
                    Ok(preset) => {
                        #[cfg(not(feature = "github-ci"))]
                        println!("[INFO] Parsing preset {path:?}");
                        return Some((path, preset));
                    }
                    Err(e) => {
                        if collect_is_error {
                            #[cfg(feature = "github-ci")]
                            println!(
                                "::error title=Failed to parse preset::{e:?} ({})",
                                path.display()
                            )
                        }
                    }
                }
            }
            return None;
        })
        .collect();

    presets
}

fn collect_all_loadable_slang_presets() -> Vec<(PathBuf, ShaderPreset)> {
    let mut presets = collect_all_slang_presets(false);
    presets.retain(|(_, preset)| {
        !preset
            .shaders
            .par_iter()
            .any(|shader| ShaderSource::load(&shader.name).is_err())
    });

    presets
}

#[test]
pub fn preprocess_all_slang_presets_parsed() {
    let presets = collect_all_slang_presets(true);

    for (path, preset) in presets {
        preset.shaders.into_par_iter().for_each(|shader| {
            if let Err(e) = ShaderSource::load(&shader.name) {
                #[cfg(not(feature = "github-ci"))]
                eprintln!(
                    "[ERROR] Failed to preprocess shader {} from preset {}: {:?}",
                    shader.name.display(),
                    path.display(),
                    e
                );

                #[cfg(feature = "github-ci")]
                println!(
                    "::warning title=Failed to preprocess shader::{e:?}, ({})",
                    path.display()
                )
            }
        })
    }
}

trait TypeDebug {
    const DEBUG: &'static str;
}

impl TypeDebug for Naga {
    const DEBUG: &'static str = "Naga";
}

impl TypeDebug for SpirvCross {
    const DEBUG: &'static str = "SpirvCross";
}

impl TypeDebug for DXIL {
    const DEBUG: &'static str = "DXIL";
}

impl TypeDebug for HLSL {
    const DEBUG: &'static str = "HLSL";
}

impl TypeDebug for WGSL {
    const DEBUG: &'static str = "WGSL";
}

impl TypeDebug for SPIRV {
    const DEBUG: &'static str = "SPIRV";
}

impl TypeDebug for GLSL {
    const DEBUG: &'static str = "GLSL";
}

impl TypeDebug for MSL {
    const DEBUG: &'static str = "MSL";
}

fn compile_presets<O: OutputTarget, R: TypeDebug>()
where
    O: Sized,
    O: FromCompilation<SpirvCompilation, R>,
    O: TypeDebug,
{
    let presets = ALL_SLANG_PRESETS.read().unwrap();
    presets.par_iter().for_each(|(path, preset)| {
        #[cfg(not(feature = "github-ci"))]
        println!(
            "[INFO] Compiling {} into {} reflecting with {}",
            path.display(),
            O::DEBUG,
            R::DEBUG
        );
        if let Err(e) = O::compile_preset_passes::<SpirvCompilation, R, Box<dyn Error>>(
            preset.shaders.clone(),
            &preset.textures,
        ) {
            #[cfg(not(feature = "github-ci"))]
            eprintln!("[ERROR] {:?} ({path:?})", e);

            #[cfg(feature = "github-ci")]
            println!(
                "::error title=Failed to reflect {} with {}::{e:?} ({})",
                O::DEBUG,
                R::DEBUG,
                path.display()
            )
        }
    });
}

#[test]
#[cfg(feature = "reflect-cross")]
pub fn compile_all_slang_presets_spirv_cross() {
    compile_presets::<SPIRV, SpirvCross>();
}

#[test]
#[cfg(feature = "reflect-cross")]
pub fn compile_all_slang_presets_hlsl_cross() {
    compile_presets::<HLSL, SpirvCross>();
}

#[test]
#[cfg(feature = "reflect-cross")]
pub fn compile_all_slang_presets_glsl_cross() {
    compile_presets::<GLSL, SpirvCross>();
}

#[test]
#[cfg(feature = "reflect-cross")]
pub fn compile_all_slang_presets_msl_cross() {
    compile_presets::<MSL, SpirvCross>();
}

#[test]
#[cfg(all(target_os = "windows", feature = "reflect-dxil"))]
pub fn compile_all_slang_presets_dxil_cross() {
    compile_presets::<DXIL, SpirvCross>();
}

#[test]
#[cfg(feature = "reflect-naga")]
pub fn compile_all_slang_presets_spirv_naga() {
    compile_presets::<SPIRV, Naga>();
}

#[test]
#[cfg(feature = "reflect-naga")]
pub fn compile_all_slang_presets_msl_naga() {
    compile_presets::<MSL, Naga>();
}

#[test]
#[cfg(feature = "reflect-naga")]
pub fn compile_all_slang_presets_wgsl_naga() {
    compile_presets::<WGSL, Naga>();
}
