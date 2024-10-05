use crate::filter_chain::FilterCommon;
use crate::framebuffer::OutputImage;
use crate::graphics_pipeline::VulkanGraphicsPipeline;
use crate::memory::RawVulkanBuffer;
use crate::options::FrameOptionsVulkan;
use crate::samplers::SamplerSet;
use crate::texture::InputImage;
use crate::{error, VulkanImage};
use ash::vk;
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_preprocess::ShaderSource;
use librashader_presets::PassMeta;
use librashader_reflect::reflect::semantics::{
    BindingStage, MemberOffset, TextureBinding, UniformBinding,
};
use librashader_reflect::reflect::ShaderReflection;
use librashader_runtime::binding::{BindSemantics, TextureInput, UniformInputs};
use librashader_runtime::filter_pass::FilterPassMeta;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::uniforms::{NoUniformBinder, UniformStorage, UniformStorageAccess};
use std::sync::Arc;

pub struct FilterPass {
    pub reflection: ShaderReflection,
    pub(crate) uniform_storage:
        UniformStorage<NoUniformBinder, Option<()>, RawVulkanBuffer, Box<[u8]>, Arc<ash::Device>>,
    pub uniform_bindings: FastHashMap<UniformBinding, MemberOffset>,
    pub source: ShaderSource,
    pub meta: PassMeta,
    pub graphics_pipeline: VulkanGraphicsPipeline,
    pub frames_in_flight: u32,
}

impl TextureInput for InputImage {
    fn size(&self) -> Size<u32> {
        self.image.size
    }
}

impl BindSemantics<NoUniformBinder, Option<()>, RawVulkanBuffer> for FilterPass {
    type InputTexture = InputImage;
    type SamplerSet = SamplerSet;
    type DescriptorSet<'a> = vk::DescriptorSet;
    type DeviceContext = Arc<ash::Device>;
    type UniformOffset = MemberOffset;

    #[inline(always)]
    fn bind_texture<'a>(
        descriptors: &mut Self::DescriptorSet<'a>,
        samplers: &Self::SamplerSet,
        binding: &TextureBinding,
        texture: &Self::InputTexture,
        device: &Self::DeviceContext,
    ) {
        let sampler = samplers.get(texture.wrap_mode, texture.filter_mode, texture.mip_filter);
        let image_info = vk::DescriptorImageInfo::default()
            .sampler(sampler.handle)
            .image_view(texture.image_view)
            .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL);

        let image_info = [image_info];
        let write_desc = vk::WriteDescriptorSet::default()
            .dst_set(*descriptors)
            .dst_binding(binding.binding)
            .dst_array_element(0)
            .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
            .image_info(&image_info);
        unsafe {
            device.update_descriptor_sets(&[write_desc], &[]);
        }
    }
}

impl FilterPassMeta for FilterPass {
    fn framebuffer_format(&self) -> ImageFormat {
        self.source.format
    }

    fn meta(&self) -> &PassMeta {
        &self.meta
    }
}

impl FilterPass {
    pub(crate) fn draw(
        &mut self,
        cmd: vk::CommandBuffer,
        format: vk::Format,
        pass_index: usize,
        parent: &FilterCommon,
        frame_count: u32,
        options: &FrameOptionsVulkan,
        viewport: &Viewport<VulkanImage>,
        original: &InputImage,
        source: &InputImage,
        output: &RenderTarget<OutputImage>,
        vbo_type: QuadType,
        use_alt_descriptors: bool,
    ) -> error::Result<Option<vk::Framebuffer>> {
        let mut descriptor = if use_alt_descriptors {
            self.graphics_pipeline.layout.descriptor_sets_alt
                [parent.internal_frame_count % self.frames_in_flight as usize]
        } else {
            self.graphics_pipeline.layout.descriptor_sets
                [parent.internal_frame_count % self.frames_in_flight as usize]
        };

        self.build_semantics(
            pass_index,
            parent,
            output.mvp,
            frame_count,
            options,
            output.output.size,
            viewport.output.size,
            &mut descriptor,
            original,
            source,
        );

        let Some(pipeline) = self
            .graphics_pipeline
            .pipelines
            .get(&format)
            .or_else(|| self.graphics_pipeline.pipelines.values().next())
        else {
            panic!("No available render pipelines found")
        };

        if let Some(ubo) = &self.reflection.ubo {
            self.uniform_storage.inner_ubo().bind_to_descriptor_set(
                descriptor,
                ubo.binding,
                &self.uniform_storage,
            )?;
        }

        output.output.begin_pass(&parent.device, cmd);

        let residual = self
            .graphics_pipeline
            .begin_rendering(output, format, cmd)?;

        unsafe {
            parent
                .device
                .cmd_bind_pipeline(cmd, vk::PipelineBindPoint::GRAPHICS, *pipeline);

            parent.device.cmd_bind_descriptor_sets(
                cmd,
                vk::PipelineBindPoint::GRAPHICS,
                self.graphics_pipeline.layout.layout,
                0,
                &[descriptor],
                &[],
            );

            if let Some(push) = &self.reflection.push_constant {
                let mut stage_mask = vk::ShaderStageFlags::empty();
                if push.stage_mask.contains(BindingStage::FRAGMENT) {
                    stage_mask |= vk::ShaderStageFlags::FRAGMENT;
                }
                if push.stage_mask.contains(BindingStage::VERTEX) {
                    stage_mask |= vk::ShaderStageFlags::VERTEX;
                }

                parent.device.cmd_push_constants(
                    cmd,
                    self.graphics_pipeline.layout.layout,
                    stage_mask,
                    0,
                    self.uniform_storage.push_slice(),
                );
            }

            parent.device.cmd_set_scissor(
                cmd,
                0,
                &[vk::Rect2D {
                    offset: vk::Offset2D {
                        x: output.x as i32,
                        y: output.y as i32,
                    },
                    extent: output.size.into(),
                }],
            );

            parent
                .device
                .cmd_set_viewport(cmd, 0, &[output.size.into()]);
            parent.draw_quad.draw_quad(&parent.device, cmd, vbo_type);
            self.graphics_pipeline.end_rendering(cmd);
        }
        Ok(residual)
    }

    fn build_semantics(
        &mut self,
        pass_index: usize,
        parent: &FilterCommon,
        mvp: &[f32; 16],
        frame_count: u32,
        options: &FrameOptionsVulkan,
        fb_size: Size<u32>,
        viewport_size: Size<u32>,
        descriptor_set: &mut vk::DescriptorSet,
        original: &InputImage,
        source: &InputImage,
    ) {
        Self::bind_semantics(
            &parent.device,
            &parent.samplers,
            &mut self.uniform_storage,
            descriptor_set,
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
            &parent.config,
        );
    }
}
