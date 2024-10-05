use crate::filter_chain::VulkanObjects;
use crate::memory::{VulkanBuffer, VulkanImageMemory};
use crate::texture::{InputImage, VulkanImage};
use crate::{error, util};
use ash::vk;
use librashader_presets::TextureMeta;
use librashader_runtime::image::{Image, BGRA8};
use librashader_runtime::scaling::MipmapSize;

pub(crate) struct LutTexture {
    _memory: VulkanImageMemory,
    _staging: VulkanBuffer,
    pub image: InputImage,
}

impl LutTexture {
    pub fn new(
        vulkan: &VulkanObjects,
        cmd: vk::CommandBuffer,
        image: Image<BGRA8>,
        config: &TextureMeta,
    ) -> error::Result<LutTexture> {
        let image_info = vk::ImageCreateInfo::default()
            .image_type(vk::ImageType::TYPE_2D)
            .format(vk::Format::B8G8R8A8_UNORM)
            .extent(image.size.into())
            .mip_levels(if config.mipmap {
                image.size.calculate_miplevels()
            } else {
                1
            })
            .array_layers(1)
            .samples(vk::SampleCountFlags::TYPE_1)
            .tiling(vk::ImageTiling::OPTIMAL)
            .usage(
                vk::ImageUsageFlags::SAMPLED
                    | vk::ImageUsageFlags::TRANSFER_SRC
                    | vk::ImageUsageFlags::TRANSFER_DST,
            )
            .initial_layout(vk::ImageLayout::UNDEFINED);

        let texture = unsafe { vulkan.device.create_image(&image_info, None)? };

        let memory = unsafe {
            let mem_reqs = vulkan.device.get_image_memory_requirements(texture);
            VulkanImageMemory::new(&vulkan.device, &vulkan.alloc, mem_reqs, &texture)?
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

        let texture_view = unsafe { vulkan.device.create_image_view(&view_info, None)? };

        let mut staging = VulkanBuffer::new(
            &vulkan.device,
            &vulkan.alloc,
            vk::BufferUsageFlags::TRANSFER_SRC,
            image.bytes.len(),
        )?;

        staging.as_mut_slice()?.copy_from_slice(&image.bytes);

        unsafe {
            util::vulkan_image_layout_transition_levels(
                &vulkan.device,
                cmd,
                texture,
                vk::REMAINING_MIP_LEVELS,
                vk::ImageLayout::UNDEFINED,
                if config.mipmap {
                    vk::ImageLayout::GENERAL
                } else {
                    vk::ImageLayout::TRANSFER_DST_OPTIMAL
                },
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

            vulkan.device.cmd_copy_buffer_to_image(
                cmd,
                staging.handle,
                texture,
                if config.mipmap {
                    vk::ImageLayout::GENERAL
                } else {
                    vk::ImageLayout::TRANSFER_DST_OPTIMAL
                },
                &[builder],
            )
        }

        // generate mipmaps
        for level in 1..image_info.mip_levels {
            let source_size = image.size.scale_mipmap(level - 1);
            let target_size = image.size.scale_mipmap(level);

            let src_offsets = [
                vk::Offset3D { x: 0, y: 0, z: 0 },
                vk::Offset3D {
                    x: source_size.width as i32,
                    y: source_size.height as i32,
                    z: 1,
                },
            ];

            let dst_offsets = [
                vk::Offset3D { x: 0, y: 0, z: 0 },
                vk::Offset3D {
                    x: target_size.width as i32,
                    y: target_size.height as i32,
                    z: 1,
                },
            ];
            let src_subresource = vk::ImageSubresourceLayers::default()
                .aspect_mask(vk::ImageAspectFlags::COLOR)
                .mip_level(level - 1)
                .base_array_layer(0)
                .layer_count(1);

            let dst_subresource = vk::ImageSubresourceLayers::default()
                .aspect_mask(vk::ImageAspectFlags::COLOR)
                .mip_level(level)
                .base_array_layer(0)
                .layer_count(1);

            let image_blit = vk::ImageBlit::default()
                .src_subresource(src_subresource)
                .src_offsets(src_offsets)
                .dst_subresource(dst_subresource)
                .dst_offsets(dst_offsets);

            unsafe {
                util::vulkan_image_layout_transition_levels(
                    &vulkan.device,
                    cmd,
                    texture,
                    vk::REMAINING_MIP_LEVELS,
                    vk::ImageLayout::GENERAL,
                    vk::ImageLayout::GENERAL,
                    vk::AccessFlags::TRANSFER_WRITE,
                    vk::AccessFlags::TRANSFER_READ,
                    vk::PipelineStageFlags::TRANSFER,
                    vk::PipelineStageFlags::TRANSFER,
                    vk::QUEUE_FAMILY_IGNORED,
                    vk::QUEUE_FAMILY_IGNORED,
                );

                // todo: respect mipmap filter?
                vulkan.device.cmd_blit_image(
                    cmd,
                    texture,
                    vk::ImageLayout::GENERAL,
                    texture,
                    vk::ImageLayout::GENERAL,
                    &[image_blit],
                    config.filter_mode.into(),
                );
            }
        }

        unsafe {
            util::vulkan_image_layout_transition_levels(
                &vulkan.device,
                cmd,
                texture,
                vk::REMAINING_MIP_LEVELS,
                if config.mipmap {
                    vk::ImageLayout::GENERAL
                } else {
                    vk::ImageLayout::TRANSFER_DST_OPTIMAL
                },
                vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                vk::AccessFlags::TRANSFER_WRITE,
                vk::AccessFlags::SHADER_READ,
                vk::PipelineStageFlags::TRANSFER,
                vk::PipelineStageFlags::FRAGMENT_SHADER,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            );
        }

        Ok(LutTexture {
            _memory: memory,
            _staging: staging,
            image: InputImage {
                image_view: texture_view,
                image: VulkanImage {
                    size: image.size,
                    image: texture,
                    format: vk::Format::B8G8R8A8_UNORM,
                },
                filter_mode: config.filter_mode,
                wrap_mode: config.wrap_mode,
                mip_filter: config.filter_mode,
            },
        })
    }
}

impl AsRef<InputImage> for LutTexture {
    fn as_ref(&self) -> &InputImage {
        &self.image
    }
}
