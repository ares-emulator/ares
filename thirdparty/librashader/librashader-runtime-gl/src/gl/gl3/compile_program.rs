use crate::binding::UniformLocation;
use crate::error;
use crate::error::FilterChainError;
use crate::gl::CompileProgram;
use crate::util;
use glow::HasContext;
use librashader_reflect::back::glsl::CrossGlslContext;
use librashader_reflect::back::ShaderCompilerOutput;
use spirv_cross2::reflect::ResourceType;
use spirv_cross2::spirv::Decoration;

pub struct Gl3CompileProgram;

impl CompileProgram for Gl3CompileProgram {
    fn compile_program(
        ctx: &glow::Context,
        glsl: ShaderCompilerOutput<String, CrossGlslContext>,
        _cache: bool,
    ) -> error::Result<(glow::Program, UniformLocation<Option<u32>>)> {
        let vertex_resources = glsl.context.artifact.vertex.shader_resources()?;

        let (program, ubo_location) = unsafe {
            let vertex = util::gl_compile_shader(ctx, glow::VERTEX_SHADER, glsl.vertex.as_str())?;
            let fragment =
                util::gl_compile_shader(ctx, glow::FRAGMENT_SHADER, glsl.fragment.as_str())?;

            let program = ctx.create_program().map_err(FilterChainError::GlError)?;
            ctx.attach_shader(program, vertex);
            ctx.attach_shader(program, fragment);

            for res in vertex_resources.resources_for_type(ResourceType::StageInput)? {
                let Some(loc) = glsl
                    .context
                    .artifact
                    .vertex
                    .decoration(res.id, Decoration::Location)?
                    .and_then(|d| d.as_literal())
                else {
                    continue;
                };

                ctx.bind_attrib_location(program, loc, &res.name.as_ref());
            }

            ctx.link_program(program);
            ctx.delete_shader(vertex);
            ctx.delete_shader(fragment);

            if !ctx.get_program_link_status(program) {
                return Err(FilterChainError::GLLinkError);
            }

            ctx.use_program(Some(program));

            for (name, binding) in &glsl.context.sampler_bindings {
                let location = ctx.get_uniform_location(program, name);
                if let Some(location) = location {
                    // eprintln!("setting sampler {location} to sample from {binding}");
                    ctx.uniform_1_i32(Some(&location), *binding as i32);
                }
            }

            ctx.use_program(None);
            (
                program,
                UniformLocation {
                    vertex: ctx.get_uniform_block_index(program, "LIBRA_UBO_VERTEX"),
                    fragment: ctx.get_uniform_block_index(program, "LIBRA_UBO_FRAGMENT"),
                },
            )
        };
        Ok((program, ubo_location))
    }
}
