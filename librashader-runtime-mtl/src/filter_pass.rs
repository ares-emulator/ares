use crate::buffer::MetalBuffer;
use crate::error;
use crate::filter_chain::FilterCommon;
use crate::graphics_pipeline::MetalGraphicsPipeline;
use crate::options::FrameOptionsMetal;
use crate::samplers::SamplerSet;
use crate::texture::{get_texture_size, InputTexture};
use icrate::Metal::{MTLCommandBuffer, MTLCommandEncoder, MTLRenderCommandEncoder, MTLTexture};
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_preprocess::ShaderSource;
use librashader_presets::ShaderPassConfig;
use librashader_reflect::reflect::semantics::{MemberOffset, TextureBinding, UniformBinding};
use librashader_reflect::reflect::ShaderReflection;
use librashader_runtime::binding::{BindSemantics, TextureInput, UniformInputs};
use librashader_runtime::filter_pass::FilterPassMeta;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::uniforms::{NoUniformBinder, UniformStorage};
use objc2::runtime::ProtocolObject;

impl TextureInput for InputTexture {
    fn size(&self) -> Size<u32> {
        get_texture_size(&self.texture)
    }
}

impl BindSemantics<NoUniformBinder, Option<()>, MetalBuffer, MetalBuffer> for FilterPass {
    type InputTexture = InputTexture;
    type SamplerSet = SamplerSet;
    type DescriptorSet<'a> = &'a ProtocolObject<dyn MTLRenderCommandEncoder>;
    type DeviceContext = ();
    type UniformOffset = MemberOffset;

    #[inline(always)]
    fn bind_texture<'a>(
        renderpass: &mut Self::DescriptorSet<'a>,
        samplers: &Self::SamplerSet,
        binding: &TextureBinding,
        texture: &Self::InputTexture,
        _device: &Self::DeviceContext,
    ) {
        let sampler = samplers.get(texture.wrap_mode, texture.filter_mode, texture.mip_filter);

        unsafe {
            renderpass.setFragmentTexture_atIndex(Some(&texture.texture), binding.binding as usize);
            renderpass.setFragmentSamplerState_atIndex(Some(sampler), binding.binding as usize);
        }
    }
}

pub struct FilterPass {
    pub reflection: ShaderReflection,
    pub(crate) uniform_storage:
        UniformStorage<NoUniformBinder, Option<()>, MetalBuffer, MetalBuffer>,
    pub uniform_bindings: FastHashMap<UniformBinding, MemberOffset>,
    pub source: ShaderSource,
    pub config: ShaderPassConfig,
    pub graphics_pipeline: MetalGraphicsPipeline,
}

impl FilterPass {
    pub(crate) fn draw(
        &mut self,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        pass_index: usize,
        parent: &FilterCommon,
        frame_count: u32,
        options: &FrameOptionsMetal,
        viewport: &Viewport<&ProtocolObject<dyn MTLTexture>>,
        original: &InputTexture,
        source: &InputTexture,
        output: &RenderTarget<ProtocolObject<dyn MTLTexture>>,
        vbo_type: QuadType,
    ) -> error::Result<()> {
        let cmd = self.graphics_pipeline.begin_rendering(output, &cmd)?;

        self.build_semantics(
            pass_index,
            parent,
            output.mvp,
            frame_count,
            options,
            get_texture_size(output.output),
            get_texture_size(viewport.output),
            original,
            source,
            &cmd,
        );

        if let Some(ubo) = &self.reflection.ubo {
            unsafe {
                cmd.setVertexBuffer_offset_atIndex(
                    Some(self.uniform_storage.inner_ubo().as_ref()),
                    0,
                    ubo.binding as usize,
                );
                cmd.setFragmentBuffer_offset_atIndex(
                    Some(self.uniform_storage.inner_ubo().as_ref()),
                    0,
                    ubo.binding as usize,
                )
            }
        }
        if let Some(pcb) = &self.reflection.push_constant {
            unsafe {
                // SPIRV-Cross always has PCB bound to 1. Naga is arbitrary but their compilation provides the next free binding for drawquad.
                cmd.setVertexBuffer_offset_atIndex(
                    Some(self.uniform_storage.inner_push().as_ref()),
                    0,
                    pcb.binding.unwrap_or(1) as usize,
                );
                cmd.setFragmentBuffer_offset_atIndex(
                    Some(self.uniform_storage.inner_push().as_ref()),
                    0,
                    pcb.binding.unwrap_or(1) as usize,
                )
            }
        }

        parent.draw_quad.draw_quad(&cmd, vbo_type);
        cmd.endEncoding();

        Ok(())
    }

    fn build_semantics<'a>(
        &mut self,
        pass_index: usize,
        parent: &FilterCommon,
        mvp: &[f32; 16],
        frame_count: u32,
        options: &FrameOptionsMetal,
        fb_size: Size<u32>,
        viewport_size: Size<u32>,
        original: &InputTexture,
        source: &InputTexture,
        mut renderpass: &ProtocolObject<dyn MTLRenderCommandEncoder>,
    ) {
        Self::bind_semantics(
            &(),
            &parent.samplers,
            &mut self.uniform_storage,
            &mut renderpass,
            UniformInputs {
                mvp,
                frame_count,
                rotation: options.rotation,
                total_subframes: options.total_subframes,
                current_subframe: options.current_subframe,
                frame_direction: options.frame_direction,
                framebuffer_size: fb_size,
                viewport_size,
            },
            original,
            source,
            &self.uniform_bindings,
            &self.reflection.meta.texture_meta,
            parent.output_textures[0..pass_index]
                .iter()
                .map(|o| o.as_ref()),
            parent.feedback_textures.iter().map(|o| o.as_ref()),
            parent.history_textures.iter().map(|o| o.as_ref()),
            parent.luts.iter().map(|(u, i)| (*u, i.as_ref())),
            &self.source.parameters,
            &parent.config.parameters,
        );

        // flush to buffers
        self.uniform_storage.inner_ubo().flush();
        self.uniform_storage.inner_push().flush();
    }
}

impl FilterPassMeta for FilterPass {
    fn framebuffer_format(&self) -> ImageFormat {
        self.source.format
    }

    fn config(&self) -> &ShaderPassConfig {
        &self.config
    }
}
