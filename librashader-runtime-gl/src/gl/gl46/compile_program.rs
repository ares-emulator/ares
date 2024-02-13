use crate::binding::UniformLocation;
use crate::error::FilterChainError;
use crate::gl::CompileProgram;
use crate::util;
use gl::types::{GLint, GLsizei, GLuint};
use librashader_cache::Cacheable;
use librashader_reflect::back::glsl::CrossGlslContext;
use librashader_reflect::back::ShaderCompilerOutput;
use spirv_cross::spirv::Decoration;

pub struct Gl4CompileProgram;

struct GlProgramBinary {
    program: Vec<u8>,
    format: GLuint,
}

impl Cacheable for GlProgramBinary {
    fn from_bytes(cached: &[u8]) -> Option<Self>
    where
        Self: Sized,
    {
        let mut cached = Vec::from(cached);
        let format = cached.split_off(cached.len() - std::mem::size_of::<u32>());
        let format: Option<&GLuint> = bytemuck::try_from_bytes(&format).ok();
        let Some(format) = format else {
            return None;
        };

        return Some(GlProgramBinary {
            program: cached,
            format: *format,
        });
    }

    fn to_bytes(&self) -> Option<Vec<u8>> {
        let mut slice = self.program.clone();
        slice.extend(bytemuck::bytes_of(&self.format));
        Some(slice)
    }
}

impl CompileProgram for Gl4CompileProgram {
    fn compile_program(
        glsl: ShaderCompilerOutput<String, CrossGlslContext>,
        cache: bool,
    ) -> crate::error::Result<(GLuint, UniformLocation<GLuint>)> {
        let vertex_resources = glsl.context.artifact.vertex.get_shader_resources()?;

        let program = librashader_cache::cache_shader_object(
            "opengl4",
            &[glsl.vertex.as_str(), glsl.fragment.as_str()],
            |&[vertex, fragment]| unsafe {
                let vertex = util::gl_compile_shader(gl::VERTEX_SHADER, vertex)?;
                let fragment = util::gl_compile_shader(gl::FRAGMENT_SHADER, fragment)?;

                let program = gl::CreateProgram();
                gl::AttachShader(program, vertex);
                gl::AttachShader(program, fragment);

                for res in &vertex_resources.stage_inputs {
                    let loc = glsl
                        .context
                        .artifact
                        .vertex
                        .get_decoration(res.id, Decoration::Location)?;
                    let mut name = res.name.clone();
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

                let mut length = 0;
                gl::GetProgramiv(program, gl::PROGRAM_BINARY_LENGTH, &mut length);

                let mut binary = vec![0; length as usize];
                let mut format = 0;
                gl::GetProgramBinary(
                    program,
                    length,
                    std::ptr::null_mut(),
                    &mut format,
                    binary.as_mut_ptr().cast(),
                );
                gl::DeleteProgram(program);
                Ok(GlProgramBinary {
                    program: binary,
                    format,
                })
            },
            |GlProgramBinary {
                 program: blob,
                 format,
             }| {
                let program = unsafe {
                    let program = gl::CreateProgram();
                    gl::ProgramBinary(program, format, blob.as_ptr().cast(), blob.len() as GLsizei);
                    program
                };

                unsafe {
                    let mut status = 0;
                    gl::GetProgramiv(program, gl::LINK_STATUS, &mut status);
                    if status != 1 {
                        return Err(FilterChainError::GLLinkError);
                    }

                    if gl::GetError() == gl::INVALID_ENUM {
                        return Err(FilterChainError::GLLinkError);
                    }
                }
                return Ok(program);
            },
            !cache,
        )?;

        let ubo_location = unsafe {
            for (name, binding) in &glsl.context.sampler_bindings {
                let location = gl::GetUniformLocation(program, name.as_str().as_ptr().cast());
                if location >= 0 {
                    gl::ProgramUniform1i(program, location, *binding as GLint);
                }
            }

            UniformLocation {
                vertex: gl::GetUniformBlockIndex(program, b"LIBRA_UBO_VERTEX\0".as_ptr().cast()),
                fragment: gl::GetUniformBlockIndex(
                    program,
                    b"LIBRA_UBO_FRAGMENT\0".as_ptr().cast(),
                ),
            }
        };

        Ok((program, ubo_location))
    }
}
