use crate::binding::UniformLocation;
use crate::error::FilterChainError;
use crate::gl::CompileProgram;
use crate::util;
use gl::types::{GLint, GLuint};
use librashader_reflect::back::glsl::CrossGlslContext;
use librashader_reflect::back::ShaderCompilerOutput;
use spirv_cross::spirv::Decoration;

pub struct Gl3CompileProgram;

impl CompileProgram for Gl3CompileProgram {
    fn compile_program(
        glsl: ShaderCompilerOutput<String, CrossGlslContext>,
        _cache: bool,
    ) -> crate::error::Result<(GLuint, UniformLocation<GLuint>)> {
        let vertex_resources = glsl.context.artifact.vertex.get_shader_resources()?;

        let (program, ubo_location) = unsafe {
            let vertex = util::gl_compile_shader(gl::VERTEX_SHADER, glsl.vertex.as_str())?;
            let fragment = util::gl_compile_shader(gl::FRAGMENT_SHADER, glsl.fragment.as_str())?;

            let program = gl::CreateProgram();
            gl::AttachShader(program, vertex);
            gl::AttachShader(program, fragment);

            for res in vertex_resources.stage_inputs {
                let loc = glsl
                    .context
                    .artifact
                    .vertex
                    .get_decoration(res.id, Decoration::Location)?;
                let mut name = res.name;
                name.push('\0');

                gl::BindAttribLocation(program, loc, name.as_str().as_ptr().cast())
            }
            gl::LinkProgram(program);
            gl::DeleteShader(vertex);
            gl::DeleteShader(fragment);

            let mut status = 0;
            gl::GetProgramiv(program, gl::LINK_STATUS, &mut status);
            if status != 1 {
                return Err(FilterChainError::GLLinkError);
            }

            gl::UseProgram(program);

            for (name, binding) in &glsl.context.sampler_bindings {
                let location = gl::GetUniformLocation(program, name.as_str().as_ptr().cast());
                if location >= 0 {
                    // eprintln!("setting sampler {location} to sample from {binding}");
                    gl::Uniform1i(location, *binding as GLint);
                }
            }

            gl::UseProgram(0);
            (
                program,
                UniformLocation {
                    vertex: gl::GetUniformBlockIndex(
                        program,
                        b"LIBRA_UBO_VERTEX\0".as_ptr().cast(),
                    ),
                    fragment: gl::GetUniformBlockIndex(
                        program,
                        b"LIBRA_UBO_FRAGMENT\0".as_ptr().cast(),
                    ),
                },
            )
        };
        Ok((program, ubo_location))
    }
}
