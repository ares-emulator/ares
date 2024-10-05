use glow::HasContext;
use librashader_reflect::reflect::ShaderReflection;

use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_preprocess::ShaderSource;
use librashader_presets::PassMeta;
use librashader_reflect::reflect::semantics::{MemberOffset, TextureBinding, UniformBinding};
use librashader_runtime::binding::{BindSemantics, ContextOffset, TextureInput, UniformInputs};
use librashader_runtime::filter_pass::FilterPassMeta;
use librashader_runtime::render_target::RenderTarget;

use crate::binding::{GlUniformBinder, GlUniformStorage, UniformLocation, VariableLocation};
use crate::filter_chain::FilterCommon;
use crate::gl::{BindTexture, GLFramebuffer, GLInterface, UboRing};
use crate::options::FrameOptionsGL;
use crate::samplers::SamplerSet;
use crate::{error, GLImage};

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
    pub program: glow::Program,
    pub ubo_location: UniformLocation<Option<u32>>,
    pub ubo_ring: Option<T::UboRing>,
    pub(crate) uniform_storage: GlUniformStorage,
    pub uniform_bindings: FastHashMap<UniformBinding, UniformOffset>,
    pub source: ShaderSource,
    pub meta: PassMeta,
}

impl TextureInput for InputTexture {
    fn size(&self) -> Size<u32> {
        self.image.size
    }
}

impl ContextOffset<GlUniformBinder, VariableLocation, glow::Context> for UniformOffset {
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
    type DeviceContext = glow::Context;
    type UniformOffset = UniformOffset;

    fn bind_texture<'a>(
        _descriptors: &mut Self::DescriptorSet<'a>,
        samplers: &Self::SamplerSet,
        binding: &TextureBinding,
        texture: &Self::InputTexture,
        device: &Self::DeviceContext,
    ) {
        T::BindTexture::bind_texture(device, samplers, binding, texture);
    }
}

impl<T: GLInterface> FilterPass<T> {
    pub(crate) fn draw(
        &mut self,
        pass_index: usize,
        parent: &FilterCommon,
        frame_count: u32,
        options: &FrameOptionsGL,
        viewport: &Viewport<&GLImage>,
        original: &InputTexture,
        source: &InputTexture,
        output: RenderTarget<GLFramebuffer, i32>,
    ) -> error::Result<()> {
        let framebuffer = output.output;

        if self.meta.mipmap_input && !parent.disable_mipmaps {
            T::BindTexture::gen_mipmaps(&parent.context, source);
        }

        unsafe {
            framebuffer.bind::<T::FramebufferInterface>()?;
            parent.context.use_program(Some(self.program));
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

        if self
            .ubo_location
            .vertex
            .is_some_and(|index| index != glow::INVALID_INDEX)
            && self
                .ubo_location
                .fragment
                .is_some_and(|index| index != glow::INVALID_INDEX)
        {
            if let (Some(ubo), Some(ring)) = (&self.reflection.ubo, &mut self.ubo_ring) {
                ring.bind_for_frame(
                    &parent.context,
                    ubo,
                    &self.ubo_location,
                    &self.uniform_storage,
                )
            }
        }

        unsafe {
            framebuffer.clear::<T::FramebufferInterface, false>();
            parent.context.viewport(
                output.x,
                output.y,
                output.size.width as i32,
                output.size.height as i32,
            );

            if framebuffer.format == glow::SRGB8_ALPHA8 {
                parent.context.enable(glow::FRAMEBUFFER_SRGB);
            } else {
                parent.context.disable(glow::FRAMEBUFFER_SRGB);
            }

            parent.context.disable(glow::CULL_FACE);
            parent.context.disable(glow::BLEND);
            parent.context.disable(glow::DEPTH_TEST);

            parent.context.draw_arrays(glow::TRIANGLE_STRIP, 0, 4);
            parent.context.disable(glow::FRAMEBUFFER_SRGB);
            parent.context.bind_framebuffer(glow::FRAMEBUFFER, None);
        }

        Ok(())
    }
}

impl<T: GLInterface> FilterPassMeta for FilterPass<T> {
    fn framebuffer_format(&self) -> ImageFormat {
        self.source.format
    }

    fn meta(&self) -> &PassMeta {
        &self.meta
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
        viewport: &Viewport<&GLImage>,
        original: &InputTexture,
        source: &InputTexture,
    ) {
        Self::bind_semantics(
            &parent.context,
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
            &parent.config,
        );
    }
}
