use glob::glob;
use librashader_preprocess::ShaderSource;
use librashader_presets::ShaderPreset;
use rayon::prelude::*;

#[test]
fn preprocess_all_slang_presets_parsed() {
    for entry in glob("../test/slang-shaders/**/*.slangp").unwrap() {
        if let Ok(path) = entry {
            if let Ok(preset) = ShaderPreset::try_parse(&path) {
                preset.shaders.into_par_iter().for_each(|shader| {
                    if let Err(e) = ShaderSource::load(&shader.name) {
                        println!(
                            "Failed to preprocess shader {} from preset {}: {:?}",
                            shader.name.display(),
                            path.display(),
                            e
                        );
                    }
                })
            }
        }
    }
}
