use gl::types::{GLint, GLsizei, GLuint};
use librashader_reflect::reflect::ShaderReflection;

use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_preprocess::ShaderSource;
use librashader_presets::ShaderPassConfig;
use librashader_reflect::reflect::semantics::{MemberOffset, TextureBinding, UniformBinding};
use librashader_runtime::binding::{BindSemantics, ContextOffset, TextureInput, UniformInputs};
use librashader_runtime::filter_pass::FilterPassMeta;
use librashader_runtime::render_target::RenderTarget;

use crate::binding::{GlUniformBinder, GlUniformStorage, UniformLocation, VariableLocation};
use crate::filter_chain::FilterCommon;
use crate::gl::{BindTexture, GLInterface, UboRing};
use crate::options::FrameOptionsGL;
use crate::samplers::SamplerSet;
use crate::GLFramebuffer;

use crate::texture::InputTexture;

pub struct UniformOffset {
    pub location: VariableLocation,
    pub offset: MemberOffset,
}

impl UniformOffset {
    pub fn new(location: VariableLocation, offset: MemberOffset) -> Self {
        Self { location, offset }
    }
}

pub(crate) struct FilterPass<T: GLInterface> {
    pub reflection: ShaderReflection,
    pub program: GLuint,
    pub ubo_location: UniformLocation<GLuint>,
    pub ubo_ring: Option<T::UboRing>,
    pub(crate) uniform_storage: GlUniformStorage,
    pub uniform_bindings: FastHashMap<UniformBinding, UniformOffset>,
    pub source: ShaderSource,
    pub config: ShaderPassConfig,
}

impl TextureInput for InputTexture {
    fn size(&self) -> Size<u32> {
        self.image.size
    }
}

impl ContextOffset<GlUniformBinder, VariableLocation> for UniformOffset {
    fn offset(&self) -> MemberOffset {
        self.offset
    }

    fn context(&self) -> VariableLocation {
        self.location
    }
}

impl<T: GLInterface> BindSemantics<GlUniformBinder, VariableLocation> for FilterPass<T> {
    type InputTexture = InputTexture;
    type SamplerSet = SamplerSet;
    type DescriptorSet<'a> = ();
    type DeviceContext = ();
    type UniformOffset = UniformOffset;

    fn bind_texture<'a>(
        _descriptors: &mut Self::DescriptorSet<'a>,
        samplers: &Self::SamplerSet,
        binding: &TextureBinding,
        texture: &Self::InputTexture,
        _device: &Self::DeviceContext,
    ) {
        T::BindTexture::bind_texture(samplers, binding, texture);
    }
}

impl<T: GLInterface> FilterPass<T> {
    pub(crate) fn draw(
        &mut self,
        pass_index: usize,
        parent: &FilterCommon,
        frame_count: u32,
        options: &FrameOptionsGL,
        viewport: &Viewport<&GLFramebuffer>,
        original: &InputTexture,
        source: &InputTexture,
        output: RenderTarget<GLFramebuffer, GLint>,
    ) {
        let framebuffer = output.output;

        if self.config.mipmap_input && !parent.disable_mipmaps {
            T::BindTexture::gen_mipmaps(source);
        }

        unsafe {
            gl::BindFramebuffer(gl::FRAMEBUFFER, framebuffer.fbo);
            gl::UseProgram(self.program);
        }

        self.build_semantics(
            pass_index,
            parent,
            output.mvp,
            frame_count,
            options,
            framebuffer.size,
            viewport,
            original,
            source,
        );

        if self.ubo_location.vertex != gl::INVALID_INDEX
            && self.ubo_location.fragment != gl::INVALID_INDEX
        {
            if let (Some(ubo), Some(ring)) = (&self.reflection.ubo, &mut self.ubo_ring) {
                ring.bind_for_frame(ubo, &self.ubo_location, &self.uniform_storage)
            }
        }

        unsafe {
            framebuffer.clear::<T::FramebufferInterface, false>();

            let framebuffer_size = framebuffer.size;
            gl::Viewport(
                output.x,
                output.y,
                framebuffer_size.width as GLsizei,
                framebuffer_size.height as GLsizei,
            );

            if framebuffer.format == gl::SRGB8_ALPHA8 {
                gl::Enable(gl::FRAMEBUFFER_SRGB);
            } else {
                gl::Disable(gl::FRAMEBUFFER_SRGB);
            }

            gl::Disable(gl::CULL_FACE);
            gl::Disable(gl::BLEND);
            gl::Disable(gl::DEPTH_TEST);

            gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
            gl::Disable(gl::FRAMEBUFFER_SRGB);
            gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
        }
    }
}

impl<T: GLInterface> FilterPassMeta for FilterPass<T> {
    fn framebuffer_format(&self) -> ImageFormat {
        self.source.format
    }

    fn config(&self) -> &ShaderPassConfig {
        &self.config
    }
}

impl<T: GLInterface> FilterPass<T> {
    // framecount should be pre-modded
    fn build_semantics(
        &mut self,
        pass_index: usize,
        parent: &FilterCommon,
        mvp: &[f32; 16],
        frame_count: u32,
        options: &FrameOptionsGL,
        fb_size: Size<u32>,
        viewport: &Viewport<&GLFramebuffer>,
        original: &InputTexture,
        source: &InputTexture,
    ) {
        Self::bind_semantics(
            &(),
            &parent.samplers,
            &mut self.uniform_storage,
            &mut (),
            UniformInputs {
                mvp,
                frame_count,
                rotation: options.rotation,
                total_subframes: options.total_subframes,
                current_subframe: options.current_subframe,
                frame_direction: options.frame_direction,
                framebuffer_size: fb_size,
                viewport_size: viewport.output.size,
            },
            original,
            source,
            &self.uniform_bindings,
            &self.reflection.meta.texture_meta,
            parent.output_textures[0..pass_index]
                .iter()
                .map(|o| o.bound()),
            parent.feedback_textures.iter().map(|o| o.bound()),
            parent.history_textures.iter().map(|o| o.bound()),
            parent.luts.iter().map(|(u, i)| (*u, i)),
            &self.source.parameters,
            &parent.config.parameters,
        );
    }
}
