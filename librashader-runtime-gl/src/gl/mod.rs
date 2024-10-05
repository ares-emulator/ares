mod framebuffer;
pub(crate) mod gl3;
pub(crate) mod gl46;

use crate::binding::UniformLocation;
use crate::error::Result;
use crate::framebuffer::GLImage;
use crate::samplers::SamplerSet;
use crate::texture::InputTexture;
pub use framebuffer::GLFramebuffer;
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size};
use librashader_presets::Scale2D;
use librashader_reflect::back::glsl::CrossGlslContext;
use librashader_reflect::back::ShaderCompilerOutput;
use librashader_reflect::reflect::semantics::{BufferReflection, TextureBinding};
use librashader_runtime::quad::{QuadType, VertexInput};
use librashader_runtime::scaling::ViewportSize;
use librashader_runtime::uniforms::UniformStorageAccess;
use std::sync::Arc;

static OFFSCREEN_VBO_DATA: &[VertexInput; 4] = &[
    VertexInput {
        position: [-1.0, -1.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    VertexInput {
        position: [1.0, -1.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
    VertexInput {
        position: [-1.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VertexInput {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

static FINAL_VBO_DATA: &[VertexInput; 4] = &[
    VertexInput {
        position: [0.0, 0.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    VertexInput {
        position: [1.0, 0.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
    VertexInput {
        position: [0.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VertexInput {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

pub(crate) trait LoadLut {
    fn load_luts(
        context: &glow::Context,
        textures: Vec<TextureResource>,
    ) -> Result<FastHashMap<usize, InputTexture>>;
}

pub(crate) trait CompileProgram {
    fn compile_program(
        context: &glow::Context,
        shader: ShaderCompilerOutput<String, CrossGlslContext>,
        cache: bool,
    ) -> Result<(glow::Program, UniformLocation<Option<u32>>)>;
}

pub(crate) trait DrawQuad {
    fn new(context: &glow::Context) -> Result<Self>
    where
        Self: Sized;
    fn bind_vertices(&self, context: &glow::Context, quad_type: QuadType);
    fn unbind_vertices(&self, context: &glow::Context);
}

pub(crate) trait UboRing<const SIZE: usize> {
    fn new(context: &glow::Context, buffer_size: u32) -> Result<Self>
    where
        Self: Sized;
    fn bind_for_frame(
        &mut self,
        context: &glow::Context,
        ubo: &BufferReflection<u32>,
        ubo_location: &UniformLocation<Option<u32>>,
        storage: &impl UniformStorageAccess,
    );
}

pub(crate) trait FramebufferInterface {
    fn new(context: &Arc<glow::Context>, max_levels: u32) -> Result<GLFramebuffer>;

    /// Create a new raw framebuffer for the given image, size, format, and miplevels.
    fn new_raw(
        context: &Arc<glow::Context>,
        image: Option<glow::Texture>,
        size: Size<u32>,
        format: u32,
        miplevels: u32,
    ) -> Result<GLFramebuffer>;

    fn scale(
        fb: &mut GLFramebuffer,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        mipmap: bool,
    ) -> Result<Size<u32>> {
        if fb.is_extern_image {
            return Ok(fb.size);
        }

        let size = source_size.scale_viewport(scaling, *viewport_size, *original_size);

        if fb.size != size
            || (mipmap && fb.max_levels == 1)
            || (!mipmap && fb.max_levels != 1)
            || fb.image.is_none()
        {
            fb.size = size;

            if mipmap {
                fb.max_levels = u32::MAX;
            } else {
                fb.max_levels = 1
            }

            Self::init(
                fb,
                size,
                if format == ImageFormat::Unknown {
                    ImageFormat::R8G8B8A8Unorm
                } else {
                    format
                },
            )?;
        }
        Ok(size)
    }

    fn clear<const REBIND: bool>(fb: &GLFramebuffer);
    fn copy_from(fb: &mut GLFramebuffer, image: &GLImage) -> Result<()>;
    fn init(fb: &mut GLFramebuffer, size: Size<u32>, format: impl Into<u32>) -> Result<()>;
    fn bind(fb: &GLFramebuffer) -> Result<()>;
}

pub(crate) trait BindTexture {
    fn bind_texture(
        context: &glow::Context,
        samplers: &SamplerSet,
        binding: &TextureBinding,
        texture: &InputTexture,
    );
    fn gen_mipmaps(context: &glow::Context, texture: &InputTexture);
}

pub(crate) trait GLInterface {
    type FramebufferInterface: FramebufferInterface;
    type UboRing: UboRing<16>;
    type DrawQuad: DrawQuad;
    type LoadLut: LoadLut;
    type BindTexture: BindTexture;
    type CompileShader: CompileProgram;
}

pub(crate) use framebuffer::OutputFramebuffer;
use librashader_pack::TextureResource;
