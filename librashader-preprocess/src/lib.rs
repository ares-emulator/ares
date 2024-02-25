//! Shader preprocessing for librashader.
//!
//! This crate contains facilities and types for resolving `#include` directives in `.slang`
//! into a single compilation unit. `#pragma` directives are also parsed and resolved as
//! [`ShaderParameter`](crate::ShaderParameter) structs.
//!
//! The resulting [`ShaderSource`](crate::ShaderSource) can then be passed into a
//! reflection target for reflection and compilation into the target shader format.
//!
//! Re-exported as [`librashader::preprocess`](https://docs.rs/librashader/latest/librashader/preprocess/index.html).
mod error;
mod include;
mod pragma;
mod stage;

use crate::include::read_source;
pub use error::*;
use librashader_common::map::FastHashMap;
use librashader_common::ImageFormat;
use std::path::Path;

/// The source file for a single shader pass.
#[derive(Debug, Clone, PartialEq)]
pub struct ShaderSource {
    /// The source contents for the vertex shader.
    pub vertex: String,

    /// The source contents for the fragment shader.
    pub fragment: String,

    /// The alias of the shader if available.
    pub name: Option<String>,

    /// The list of shader parameters found in the shader source.
    pub parameters: FastHashMap<String, ShaderParameter>,

    /// The image format the shader expects.
    pub format: ImageFormat,
}

/// A user tweakable parameter for the shader as declared in source.
#[derive(Debug, Clone, PartialEq)]
pub struct ShaderParameter {
    /// The name of the parameter.
    pub id: String,
    /// The description of the parameter.
    pub description: String,
    /// The initial value the parameter is set to.
    pub initial: f32,
    /// The minimum value that the parameter can be set to.
    pub minimum: f32,
    /// The maximum value that the parameter can be set to.
    pub maximum: f32,
    /// The step by which this parameter can be incremented or decremented.
    pub step: f32,
}

impl ShaderSource {
    /// Load the source file at the given path, resolving includes relative to the location of the
    /// source file.
    pub fn load(path: impl AsRef<Path>) -> Result<ShaderSource, PreprocessError> {
        load_shader_source(path)
    }
}

pub(crate) trait SourceOutput {
    fn push_line(&mut self, str: &str);
    fn mark_line(&mut self, line_no: usize, comment: &str) {
        #[cfg(feature = "line_directives")]
        self.push_line(&format!("#line {line_no} \"{comment}\""))
    }
}

impl SourceOutput for String {
    fn push_line(&mut self, str: &str) {
        self.push_str(str);
        self.push('\n');
    }
}

pub(crate) fn load_shader_source(path: impl AsRef<Path>) -> Result<ShaderSource, PreprocessError> {
    let source = read_source(path)?;
    let meta = pragma::parse_pragma_meta(&source)?;
    let text = stage::process_stages(&source)?;
    let parameters = FastHashMap::from_iter(meta.parameters.into_iter().map(|p| (p.id.clone(), p)));

    Ok(ShaderSource {
        vertex: text.vertex,
        fragment: text.fragment,
        name: meta.name,
        parameters,
        format: meta.format,
    })
}

#[cfg(test)]
mod test {
    use crate::include::read_source;
    use crate::{load_shader_source, pragma};

    #[test]
    pub fn load_file() {
        let result = load_shader_source(
            "../test/slang-shaders/blurs/shaders/royale/blur3x3-last-pass.slang",
        )
        .unwrap();
        eprintln!("{:#}", result.vertex)
    }

    #[test]
    pub fn preprocess_file() {
        let result =
            read_source("../test/slang-shaders/blurs/shaders/royale/blur3x3-last-pass.slang")
                .unwrap();
        eprintln!("{result}")
    }

    #[test]
    pub fn get_param_pragmas() {
        let result = read_source(
            "../test/slang-shaders/crt/shaders/crt-maximus-royale/src/ntsc_pass1.slang",
        )
        .unwrap();

        let params = pragma::parse_pragma_meta(result).unwrap();
        eprintln!("{params:?}")
    }
}
