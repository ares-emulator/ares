use ash::ext::debug_utils::Instance;
use ash::prelude::VkResult;
use ash::vk;
use ash::vk::{DebugUtilsMessengerEXT, PFN_vkDebugUtilsMessengerCallbackEXT};

pub struct VulkanDebug {
    pub loader: Instance,
    messenger: DebugUtilsMessengerEXT,
}

#[allow(dead_code)]
impl VulkanDebug {
    pub fn new(
        entry: &ash::Entry,
        instance: &ash::Instance,
        callback: PFN_vkDebugUtilsMessengerCallbackEXT,
    ) -> VkResult<VulkanDebug> {
        let debug_info = vk::DebugUtilsMessengerCreateInfoEXT::default()
            .message_severity(
                vk::DebugUtilsMessageSeverityFlagsEXT::ERROR
                    | vk::DebugUtilsMessageSeverityFlagsEXT::WARNING
                    | vk::DebugUtilsMessageSeverityFlagsEXT::INFO,
            )
            .message_type(
                vk::DebugUtilsMessageTypeFlagsEXT::GENERAL
                    | vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION
                    | vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE,
            )
            .pfn_user_callback(callback);

        let debug_utils_loader = Instance::new(entry, instance);

        dbg!("got to dbg");
        unsafe {
            let debug_call_back =
                debug_utils_loader.create_debug_utils_messenger(&debug_info, None)?;

            Ok(VulkanDebug {
                loader: debug_utils_loader,
                messenger: debug_call_back,
            })
        }
    }
}

impl Drop for VulkanDebug {
    fn drop(&mut self) {
        unsafe {
            self.loader
                .destroy_debug_utils_messenger(self.messenger, None);
        }
    }
}
