use crate::error::ShaderCompileError;
use glslang::{CompilerOptions, ShaderInput};
use librashader_preprocess::ShaderSource;

use crate::front::{ShaderInputCompiler, SpirvCompilation};
#[cfg(feature = "serialize")]
use serde::{Deserialize, Serialize};

/// glslang compiler
pub struct Glslang;

impl ShaderInputCompiler<SpirvCompilation> for Glslang {
    fn compile(source: &ShaderSource) -> Result<SpirvCompilation, ShaderCompileError> {
        compile_spirv(source)
    }
}

pub(crate) fn compile_spirv(source: &ShaderSource) -> Result<SpirvCompilation, ShaderCompileError> {
    let compiler = glslang::Compiler::acquire().ok_or(ShaderCompileError::CompilerInitError)?;
    let options = CompilerOptions {
        source_language: glslang::SourceLanguage::GLSL,
        target: glslang::Target::Vulkan {
            version: glslang::VulkanVersion::Vulkan1_0,
            spirv_version: glslang::SpirvVersion::SPIRV1_0,
        },
        version_profile: None,
    };

    let vertex = glslang::ShaderSource::from(source.vertex.as_str());
    let vertex = ShaderInput::new(&vertex, glslang::ShaderStage::Vertex, &options, None)?;
    let vertex = compiler.create_shader(vertex)?;

    let fragment = glslang::ShaderSource::from(source.fragment.as_str());
    let fragment = ShaderInput::new(&fragment, glslang::ShaderStage::Fragment, &options, None)?;
    let fragment = compiler.create_shader(fragment)?;

    let vertex = Vec::from(vertex.compile()?);
    let fragment = Vec::from(fragment.compile()?);

    Ok(SpirvCompilation { vertex, fragment })
}

#[cfg(test)]
mod test {
    use crate::front::glslang::compile_spirv;
    use librashader_preprocess::ShaderSource;
    #[test]
    pub fn compile_shader() {
        let result = ShaderSource::load("../test/basic.slang").unwrap();
        let _spirv = compile_spirv(&result).unwrap();
    }
}
