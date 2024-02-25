use gl::types::{GLenum, GLuint};

use crate::error;
use crate::error::FilterChainError;
use librashader_reflect::back::glsl::GlslVersion;

pub unsafe fn gl_compile_shader(stage: GLenum, source: &str) -> error::Result<GLuint> {
    let (shader, compile_status) = unsafe {
        let shader = gl::CreateShader(stage);
        gl::ShaderSource(
            shader,
            1,
            &source.as_bytes().as_ptr().cast(),
            std::ptr::null(),
        );
        gl::CompileShader(shader);
        let mut compile_status = 0;
        gl::GetShaderiv(shader, gl::COMPILE_STATUS, &mut compile_status);
        (shader, compile_status)
    };

    if compile_status == 0 {
        Err(FilterChainError::GlCompileError)
    } else {
        Ok(shader)
    }
}

pub fn gl_get_version() -> GlslVersion {
    let mut maj_ver = 0;
    let mut min_ver = 0;
    unsafe {
        gl::GetIntegerv(gl::MAJOR_VERSION, &mut maj_ver);
        gl::GetIntegerv(gl::MINOR_VERSION, &mut min_ver);
    }

    match maj_ver {
        3 => match min_ver {
            3 => GlslVersion::V3_30,
            2 => GlslVersion::V1_50,
            1 => GlslVersion::V1_40,
            0 => GlslVersion::V1_30,
            _ => GlslVersion::V1_50,
        },
        4 => match min_ver {
            6 => GlslVersion::V4_60,
            5 => GlslVersion::V4_50,
            4 => GlslVersion::V4_40,
            3 => GlslVersion::V4_30,
            2 => GlslVersion::V4_20,
            1 => GlslVersion::V4_10,
            0 => GlslVersion::V4_00,
            _ => GlslVersion::V1_50,
        },
        _ => GlslVersion::V1_50,
    }
}

pub fn gl_u16_to_version(version: u16) -> GlslVersion {
    match version {
        0 => gl_get_version(),
        300 => GlslVersion::V1_30,
        310 => GlslVersion::V1_40,
        320 => GlslVersion::V1_50,
        330 => GlslVersion::V3_30,
        400 => GlslVersion::V4_00,
        410 => GlslVersion::V4_10,
        420 => GlslVersion::V4_20,
        430 => GlslVersion::V4_30,
        440 => GlslVersion::V4_40,
        450 => GlslVersion::V4_50,
        460 => GlslVersion::V4_60,
        _ => GlslVersion::V1_50,
    }
}
