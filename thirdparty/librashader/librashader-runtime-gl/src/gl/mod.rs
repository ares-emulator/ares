mod framebuffer;
pub(crate) mod gl3;
pub(crate) mod gl46;

use crate::binding::UniformLocation;
use crate::error::Result;
use crate::framebuffer::GLImage;
use crate::samplers::SamplerSet;
use crate::texture::InputTexture;
use bytemuck::{Pod, Zeroable};
pub use framebuffer::GLFramebuffer;
use gl::types::{GLenum, GLuint};
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size};
use librashader_presets::{Scale2D, TextureConfig};
use librashader_reflect::back::glsl::CrossGlslContext;
use librashader_reflect::back::ShaderCompilerOutput;
use librashader_reflect::reflect::semantics::{BufferReflection, TextureBinding};
use librashader_runtime::quad::QuadType;
use librashader_runtime::uniforms::UniformStorageAccess;

#[repr(C)]
#[derive(Debug, Copy, Clone, Default, Zeroable, Pod)]

pub(crate) struct OpenGLVertex {
    pub position: [f32; 4],
    pub texcoord: [f32; 2],
}

static OFFSCREEN_VBO_DATA: &[OpenGLVertex; 4] = &[
    OpenGLVertex {
        position: [-1.0, -1.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    OpenGLVertex {
        position: [1.0, -1.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
    OpenGLVertex {
        position: [-1.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    OpenGLVertex {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

static FINAL_VBO_DATA: &[OpenGLVertex; 4] = &[
    OpenGLVertex {
        position: [0.0, 0.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    OpenGLVertex {
        position: [1.0, 0.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
    OpenGLVertex {
        position: [0.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    OpenGLVertex {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

pub(crate) trait LoadLut {
    fn load_luts(textures: &[TextureConfig]) -> Result<FastHashMap<usize, InputTexture>>;
}

pub(crate) trait CompileProgram {
    fn compile_program(
        shader: ShaderCompilerOutput<String, CrossGlslContext>,
        cache: bool,
    ) -> Result<(GLuint, UniformLocation<GLuint>)>;
}

pub(crate) trait DrawQuad {
    fn new() -> Self;
    fn bind_vertices(&self, quad_type: QuadType);
    fn unbind_vertices(&self);
}

pub(crate) trait UboRing<const SIZE: usize> {
    fn new(buffer_size: u32) -> Self;
    fn bind_for_frame(
        &mut self,
        ubo: &BufferReflection<u32>,
        ubo_location: &UniformLocation<GLuint>,
        storage: &impl UniformStorageAccess,
    );
}

pub(crate) trait FramebufferInterface {
    fn new(max_levels: u32) -> GLFramebuffer;
    fn scale(
        fb: &mut GLFramebuffer,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        mipmap: bool,
    ) -> Result<Size<u32>>;
    fn clear<const REBIND: bool>(fb: &GLFramebuffer);
    fn copy_from(fb: &mut GLFramebuffer, image: &GLImage) -> Result<()>;
    fn init(fb: &mut GLFramebuffer, size: Size<u32>, format: impl Into<GLenum>) -> Result<()>;
}

pub(crate) trait BindTexture {
    fn bind_texture(samplers: &SamplerSet, binding: &TextureBinding, texture: &InputTexture);
    fn gen_mipmaps(texture: &InputTexture);
}

pub(crate) trait GLInterface {
    type FramebufferInterface: FramebufferInterface;
    type UboRing: UboRing<16>;
    type DrawQuad: DrawQuad;
    type LoadLut: LoadLut;
    type BindTexture: BindTexture;
    type CompileShader: CompileProgram;
}
