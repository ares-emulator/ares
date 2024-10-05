use glow::HasContext;

use crate::error;
use crate::error::FilterChainError;
use librashader_reflect::back::glsl::GlslVersion;

pub fn gl_compile_shader(
    context: &glow::Context,
    stage: u32,
    source: &str,
) -> error::Result<glow::Shader> {
    unsafe {
        let shader = context
            .create_shader(stage)
            .map_err(|_| FilterChainError::GlCompileError)?;

        context.shader_source(shader, &source);
        context.compile_shader(shader);
        let compile_status = context.get_shader_compile_status(shader);

        if !compile_status {
            Err(FilterChainError::GlCompileError)
        } else {
            Ok(shader)
        }
    }
}

pub fn gl_get_version(context: &glow::Context) -> GlslVersion {
    let version = context.version();

    let maj_ver = version.major;
    let min_ver = version.minor;

    match maj_ver {
        3 => match min_ver {
            3 => GlslVersion::Glsl330,
            2 => GlslVersion::Glsl150,
            1 => GlslVersion::Glsl140,
            0 => GlslVersion::Glsl130,
            _ => GlslVersion::Glsl150,
        },
        4 => match min_ver {
            6 => GlslVersion::Glsl460,
            5 => GlslVersion::Glsl450,
            4 => GlslVersion::Glsl440,
            3 => GlslVersion::Glsl430,
            2 => GlslVersion::Glsl420,
            1 => GlslVersion::Glsl410,
            0 => GlslVersion::Glsl400,
            _ => GlslVersion::Glsl150,
        },
        _ => GlslVersion::Glsl150,
    }
}

pub fn gl_u16_to_version(context: &glow::Context, version: u16) -> GlslVersion {
    match version {
        0 => gl_get_version(context),
        300 => GlslVersion::Glsl130,
        310 => GlslVersion::Glsl140,
        320 => GlslVersion::Glsl150,
        330 => GlslVersion::Glsl330,
        400 => GlslVersion::Glsl400,
        410 => GlslVersion::Glsl410,
        420 => GlslVersion::Glsl420,
        430 => GlslVersion::Glsl430,
        440 => GlslVersion::Glsl440,
        450 => GlslVersion::Glsl450,
        460 => GlslVersion::Glsl460,
        _ => GlslVersion::Glsl150,
    }
}
