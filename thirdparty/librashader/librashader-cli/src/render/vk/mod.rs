use crate::render::vk::base::VulkanBase;
use crate::render::vk::memory::{VulkanBuffer, VulkanImageMemory};
use crate::render::{CommonFrameOptions, RenderTest};
use anyhow::anyhow;
use ash::vk;
use gpu_allocator::MemoryLocation;
use image::RgbaImage;
use librashader::presets::ShaderPreset;
use librashader::runtime::vk::{FilterChain, FilterChainOptions, FrameOptions, VulkanImage};
use librashader::runtime::{FilterChainParameters, RuntimeParameters};
use librashader::runtime::{Size, Viewport};
use librashader_runtime::image::{Image, UVDirection, BGRA8};
use std::path::Path;

mod base;
mod memory;
mod physical_device;
mod util;

pub struct Vulkan {
    vk: VulkanBase,
    image_bytes: Image<BGRA8>,
    image: vk::Image,
    _image_alloc: VulkanImageMemory,
}

impl RenderTest for Vulkan {
    fn new(path: &Path) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        Vulkan::new(path)
    }

    fn image_size(&self) -> Size<u32> {
        self.image_bytes.size
    }

    fn render_with_preset_and_params(
        &mut self,
        preset: ShaderPreset,
        frame_count: usize,
        output_size: Option<Size<u32>>,
        param_setter: Option<&dyn Fn(&RuntimeParameters)>,
        frame_options: Option<CommonFrameOptions>,
    ) -> anyhow::Result<image::RgbaImage> {
        unsafe {
            let mut filter_chain = FilterChain::load_from_preset(
                preset,
                &self.vk,
                Some(&FilterChainOptions {
                    frames_in_flight: 3,
                    force_no_mipmaps: false,
                    use_dynamic_rendering: false,
                    disable_cache: false,
                }),
            )?;

            if let Some(setter) = param_setter {
                setter(filter_chain.parameters());
            }

            let image_info = vk::ImageCreateInfo::default()
                .image_type(vk::ImageType::TYPE_2D)
                .format(vk::Format::B8G8R8A8_UNORM)
                .extent(output_size.map_or(self.image_bytes.size.into(), |size| size.into()))
                .mip_levels(1)
                .array_layers(1)
                .samples(vk::SampleCountFlags::TYPE_1)
                .tiling(vk::ImageTiling::OPTIMAL)
                .usage(vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::TRANSFER_SRC)
                .initial_layout(vk::ImageLayout::UNDEFINED);

            let render_texture = { self.vk.device().create_image(&image_info, None)? };

            // This just needs to stay alive until the read.
            let _memory = {
                let mem_reqs = self
                    .vk
                    .device()
                    .get_image_memory_requirements(render_texture);
                VulkanImageMemory::new(
                    self.vk.device(),
                    &self.vk.allocator(),
                    mem_reqs,
                    &render_texture,
                    MemoryLocation::GpuOnly,
                )?
            };

            let transfer_texture = self.vk.device().create_image(
                &vk::ImageCreateInfo::default()
                    .image_type(vk::ImageType::TYPE_2D)
                    .format(vk::Format::R8G8B8A8_UNORM)
                    .extent(self.image_bytes.size.into())
                    .mip_levels(1)
                    .array_layers(1)
                    .samples(vk::SampleCountFlags::TYPE_1)
                    .tiling(vk::ImageTiling::LINEAR)
                    .usage(vk::ImageUsageFlags::TRANSFER_DST)
                    .initial_layout(vk::ImageLayout::UNDEFINED),
                None,
            )?;

            let mut transfer_memory = {
                let mem_reqs = self
                    .vk
                    .device()
                    .get_image_memory_requirements(transfer_texture);
                VulkanImageMemory::new(
                    self.vk.device(),
                    &self.vk.allocator(),
                    mem_reqs,
                    &transfer_texture,
                    MemoryLocation::GpuToCpu,
                )?
            };

            self.vk.queue_work(|cmd| {
                util::vulkan_image_layout_transition_levels(
                    &self.vk.device(),
                    cmd,
                    render_texture,
                    vk::REMAINING_MIP_LEVELS,
                    vk::ImageLayout::UNDEFINED,
                    vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                    vk::AccessFlags::COLOR_ATTACHMENT_WRITE,
                    vk::AccessFlags::COLOR_ATTACHMENT_WRITE,
                    vk::PipelineStageFlags::ALL_GRAPHICS,
                    vk::PipelineStageFlags::ALL_GRAPHICS,
                    vk::QUEUE_FAMILY_IGNORED,
                    vk::QUEUE_FAMILY_IGNORED,
                );

                util::vulkan_image_layout_transition_levels(
                    &self.vk.device(),
                    cmd,
                    transfer_texture,
                    vk::REMAINING_MIP_LEVELS,
                    vk::ImageLayout::UNDEFINED,
                    vk::ImageLayout::GENERAL,
                    vk::AccessFlags::TRANSFER_WRITE | vk::AccessFlags::TRANSFER_READ,
                    vk::AccessFlags::TRANSFER_WRITE | vk::AccessFlags::TRANSFER_READ,
                    vk::PipelineStageFlags::ALL_GRAPHICS,
                    vk::PipelineStageFlags::TRANSFER,
                    vk::QUEUE_FAMILY_IGNORED,
                    vk::QUEUE_FAMILY_IGNORED,
                );

                let options = frame_options.map(|options| FrameOptions {
                    clear_history: options.clear_history,
                    frame_direction: options.frame_direction,
                    rotation: options.rotation,
                    total_subframes: options.total_subframes,
                    current_subframe: options.current_subframe,
                });

                let viewport = Viewport::new_render_target_sized_origin(
                    VulkanImage {
                        image: render_texture,
                        size: self.image_bytes.size.into(),
                        format: vk::Format::B8G8R8A8_UNORM,
                    },
                    None,
                )?;

                for frame in 0..=frame_count {
                    filter_chain.frame(
                        &VulkanImage {
                            image: self.image,
                            size: self.image_bytes.size,
                            format: vk::Format::B8G8R8A8_UNORM,
                        },
                        &viewport,
                        cmd,
                        frame,
                        options.as_ref(),
                    )?;
                }

                {
                    util::vulkan_image_layout_transition_levels(
                        &self.vk.device(),
                        cmd,
                        render_texture,
                        vk::REMAINING_MIP_LEVELS,
                        vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                        vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
                        vk::AccessFlags::COLOR_ATTACHMENT_WRITE,
                        vk::AccessFlags::TRANSFER_READ,
                        vk::PipelineStageFlags::ALL_GRAPHICS,
                        vk::PipelineStageFlags::TRANSFER,
                        vk::QUEUE_FAMILY_IGNORED,
                        vk::QUEUE_FAMILY_IGNORED,
                    )
                }

                let offsets = [
                    vk::Offset3D { x: 0, y: 0, z: 0 },
                    vk::Offset3D {
                        x: self.image_bytes.size.width as i32,
                        y: self.image_bytes.size.height as i32,
                        z: 1,
                    },
                ];

                let subresource = vk::ImageSubresourceLayers::default()
                    .aspect_mask(vk::ImageAspectFlags::COLOR)
                    .base_array_layer(0)
                    .layer_count(1);

                let image_blit = vk::ImageBlit::default()
                    .src_subresource(subresource.clone())
                    .src_offsets(offsets.clone())
                    .dst_subresource(subresource)
                    .dst_offsets(offsets);

                self.vk.device().cmd_blit_image(
                    cmd,
                    render_texture,
                    vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
                    transfer_texture,
                    vk::ImageLayout::GENERAL,
                    &[image_blit],
                    vk::Filter::NEAREST,
                );

                Ok::<_, anyhow::Error>(())
            })??;

            // should have read now.
            let mut memory = transfer_memory
                .allocation
                .mapped_slice_mut()
                .ok_or(anyhow!("readback buffer was not mapped"))?;

            let layout = self.vk.device().get_image_subresource_layout(
                transfer_texture,
                vk::ImageSubresource::default()
                    .aspect_mask(vk::ImageAspectFlags::COLOR)
                    .array_layer(0),
            );
            memory = &mut memory[layout.offset as usize..];

            // let mut pixels = Vec::with_capacity(layout.size as usize);

            let image = RgbaImage::from_raw(
                self.image_bytes.size.width,
                self.image_bytes.size.height,
                Vec::from(memory),
            )
            .ok_or(anyhow!("failed to create image from slice"));

            self.vk.device().destroy_image(transfer_texture, None);
            self.vk.device().destroy_image(render_texture, None);

            Ok(image?)
        }
    }
}

impl Vulkan {
    pub fn new(image_path: &Path) -> anyhow::Result<Self> {
        let vk = VulkanBase::new()?;

        let (image_bytes, image_alloc, image, _view) = Self::load_image(&vk, image_path)?;

        Ok(Self {
            vk,
            image,
            image_bytes,
            _image_alloc: image_alloc,
        })
    }

    pub fn load_image(
        vk: &VulkanBase,
        image_path: &Path,
    ) -> anyhow::Result<(Image<BGRA8>, VulkanImageMemory, vk::Image, vk::ImageView)> {
        let image: Image<BGRA8> = Image::load(image_path, UVDirection::TopLeft)?;

        let image_info = vk::ImageCreateInfo::default()
            .image_type(vk::ImageType::TYPE_2D)
            .format(vk::Format::B8G8R8A8_UNORM)
            .extent(image.size.into())
            .mip_levels(1)
            .array_layers(1)
            .samples(vk::SampleCountFlags::TYPE_1)
            .tiling(vk::ImageTiling::OPTIMAL)
            .usage(
                vk::ImageUsageFlags::SAMPLED
                    | vk::ImageUsageFlags::TRANSFER_SRC
                    | vk::ImageUsageFlags::TRANSFER_DST,
            )
            .initial_layout(vk::ImageLayout::UNDEFINED);

        let texture = unsafe { vk.device().create_image(&image_info, None)? };

        let memory = unsafe {
            let mem_reqs = vk.device().get_image_memory_requirements(texture);
            VulkanImageMemory::new(
                vk.device(),
                &vk.allocator(),
                mem_reqs,
                &texture,
                MemoryLocation::GpuOnly,
            )?
        };

        let image_subresource = vk::ImageSubresourceRange::default()
            .level_count(image_info.mip_levels)
            .layer_count(1)
            .aspect_mask(vk::ImageAspectFlags::COLOR);

        let swizzle_components = vk::ComponentMapping::default()
            .r(vk::ComponentSwizzle::R)
            .g(vk::ComponentSwizzle::G)
            .b(vk::ComponentSwizzle::B)
            .a(vk::ComponentSwizzle::A);

        let view_info = vk::ImageViewCreateInfo::default()
            .view_type(vk::ImageViewType::TYPE_2D)
            .format(vk::Format::B8G8R8A8_UNORM)
            .image(texture)
            .subresource_range(image_subresource)
            .components(swizzle_components);

        let texture_view = unsafe { vk.device().create_image_view(&view_info, None)? };

        let mut staging = VulkanBuffer::new(
            &vk.device(),
            &vk.allocator(),
            vk::BufferUsageFlags::TRANSFER_SRC,
            image.bytes.len(),
        )?;

        staging.as_mut_slice()?.copy_from_slice(&image.bytes);

        vk.queue_work(|cmd| unsafe {
            util::vulkan_image_layout_transition_levels(
                &vk.device(),
                cmd,
                texture,
                vk::REMAINING_MIP_LEVELS,
                vk::ImageLayout::UNDEFINED,
                vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                vk::AccessFlags::empty(),
                vk::AccessFlags::TRANSFER_WRITE,
                vk::PipelineStageFlags::TOP_OF_PIPE,
                vk::PipelineStageFlags::TRANSFER,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            );

            let builder = vk::BufferImageCopy::default()
                .image_subresource(
                    vk::ImageSubresourceLayers::default()
                        .aspect_mask(vk::ImageAspectFlags::COLOR)
                        .mip_level(0)
                        .base_array_layer(0)
                        .layer_count(1),
                )
                .image_extent(image.size.into());

            vk.device().cmd_copy_buffer_to_image(
                cmd,
                staging.handle,
                texture,
                vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                &[builder],
            );

            util::vulkan_image_layout_transition_levels(
                &vk.device(),
                cmd,
                texture,
                vk::REMAINING_MIP_LEVELS,
                vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                vk::AccessFlags::TRANSFER_WRITE,
                vk::AccessFlags::SHADER_READ,
                vk::PipelineStageFlags::TRANSFER,
                vk::PipelineStageFlags::ALL_GRAPHICS,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            );
        })?;

        Ok((image, memory, texture, texture_view))
    }
}
