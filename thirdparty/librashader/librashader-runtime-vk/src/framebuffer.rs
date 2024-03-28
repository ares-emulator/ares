use crate::texture::VulkanImage;
use crate::{error, util};
use ash::vk;
use librashader_common::Size;

#[derive(Clone)]
pub(crate) struct OutputImage {
    pub size: Size<u32>,
    pub image_view: vk::ImageView,
    image: vk::Image,
}

impl OutputImage {
    pub fn new(device: &ash::Device, image: VulkanImage) -> error::Result<OutputImage> {
        let image_subresource = vk::ImageSubresourceRange::builder()
            .base_mip_level(0)
            .base_array_layer(0)
            .level_count(1)
            .layer_count(1)
            .aspect_mask(vk::ImageAspectFlags::COLOR);

        let swizzle_components = vk::ComponentMapping::builder()
            .r(vk::ComponentSwizzle::R)
            .g(vk::ComponentSwizzle::G)
            .b(vk::ComponentSwizzle::B)
            .a(vk::ComponentSwizzle::A);

        let view_info = vk::ImageViewCreateInfo::builder()
            .view_type(vk::ImageViewType::TYPE_2D)
            .format(image.format)
            .image(image.image)
            .subresource_range(*image_subresource)
            .components(*swizzle_components);

        let image_view = unsafe { device.create_image_view(&view_info, None)? };

        Ok(OutputImage {
            size: image.size,
            image: image.image,
            image_view,
        })
    }

    pub fn begin_pass(&self, device: &ash::Device, cmd: vk::CommandBuffer) {
        unsafe {
            util::vulkan_image_layout_transition_levels(
                device,
                cmd,
                self.image,
                1,
                vk::ImageLayout::UNDEFINED,
                vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                vk::AccessFlags::empty(),
                vk::AccessFlags::COLOR_ATTACHMENT_READ | vk::AccessFlags::COLOR_ATTACHMENT_WRITE,
                vk::PipelineStageFlags::ALL_GRAPHICS,
                vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            )
        }
    }

    pub fn end_pass(&self, device: &ash::Device, cmd: vk::CommandBuffer) {
        unsafe {
            util::vulkan_image_layout_transition_levels(
                &device,
                cmd,
                self.image,
                vk::REMAINING_MIP_LEVELS,
                vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                vk::AccessFlags::COLOR_ATTACHMENT_WRITE,
                vk::AccessFlags::SHADER_READ,
                vk::PipelineStageFlags::ALL_GRAPHICS,
                vk::PipelineStageFlags::FRAGMENT_SHADER,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            )
        }
    }
}
