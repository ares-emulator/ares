use ash::prelude::VkResult;
use ash::vk;

pub struct VulkanFramebuffer {
    pub framebuffer: vk::Framebuffer,
}

impl VulkanFramebuffer {
    pub fn new(
        device: &ash::Device,
        image_view: &vk::ImageView,
        render_pass: &vk::RenderPass,
        width: u32,
        height: u32,
    ) -> VkResult<VulkanFramebuffer> {
        let attachments = &[*image_view];
        let framebuffer_info = vk::FramebufferCreateInfo::default()
            .render_pass(*render_pass)
            .attachments(attachments)
            .width(width)
            .height(height)
            .layers(1);

        unsafe {
            let framebuffer = device.create_framebuffer(&framebuffer_info, None)?;
            Ok(VulkanFramebuffer { framebuffer })
        }
    }
}
