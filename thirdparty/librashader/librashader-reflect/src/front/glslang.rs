use crate::error::ShaderCompileError;
use glslang::{CompilerOptions, ShaderInput};
use librashader_preprocess::ShaderSource;
use rspirv::binary::Assemble;
use rspirv::dr::Builder;

use crate::front::spirv_passes::{link_input_outputs, load_module};
use crate::front::{ShaderInputCompiler, SpirvCompilation};

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

    let vertex = vertex.compile()?;
    let fragment = fragment.compile()?;

    let vertex = load_module(&vertex);
    let fragment = load_module(&fragment);
    let mut fragment = Builder::new_from_module(fragment);

    let mut pass = link_input_outputs::LinkInputs::new(&vertex, &mut fragment);
    pass.do_pass();

    let vertex = vertex.assemble();
    let fragment = fragment.module().assemble();

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
