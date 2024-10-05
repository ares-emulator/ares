use crate::hello_triangle::vulkan_base::VulkanBase;
use ash::prelude::VkResult;
use ash::vk;
use raw_window_handle::{HasRawDisplayHandle, HasRawWindowHandle};

pub struct VulkanSurface {
    surface_loader: ash::khr::surface::Instance,
    pub surface: vk::SurfaceKHR,
    pub present_queue: vk::Queue,
}

impl VulkanSurface {
    pub fn new(base: &VulkanBase, window: &winit::window::Window) -> VkResult<VulkanSurface> {
        let surface = unsafe {
            ash_window::create_surface(
                &base.entry,
                &base.instance,
                window.raw_display_handle().unwrap(),
                window.raw_window_handle().unwrap(),
                None,
            )?
        };

        let surface_loader = ash::khr::surface::Instance::new(&base.entry, &base.instance);

        let present_queue = unsafe {
            let queue_family = base
                .instance
                .get_physical_device_queue_family_properties(base.physical_device)
                .iter()
                .enumerate()
                .find_map(|(index, info)| {
                    let supports_graphic_and_surface =
                        info.queue_flags.contains(vk::QueueFlags::GRAPHICS)
                            && surface_loader
                                .get_physical_device_surface_support(
                                    base.physical_device,
                                    index as u32,
                                    surface,
                                )
                                .unwrap();
                    if supports_graphic_and_surface {
                        Some(index)
                    } else {
                        None
                    }
                })
                .expect("couldn't find suitable device");
            base.device.get_device_queue(queue_family as u32, 0)
        };

        Ok(VulkanSurface {
            surface,
            surface_loader,
            present_queue,
        })
    }

    pub fn choose_format(&self, base: &VulkanBase) -> VkResult<vk::SurfaceFormatKHR> {
        unsafe {
            let available_formats = self
                .surface_loader
                .get_physical_device_surface_formats(base.physical_device, self.surface)?;
            for available_format in available_formats.iter() {
                if available_format.format == vk::Format::B8G8R8A8_SRGB
                    && available_format.color_space == vk::ColorSpaceKHR::SRGB_NONLINEAR
                {
                    return Ok(*available_format);
                }
            }

            return Ok(*available_formats.first().unwrap());
        }
    }

    pub fn choose_present_mode(&self, base: &VulkanBase) -> VkResult<vk::PresentModeKHR> {
        unsafe {
            let available_formats = self
                .surface_loader
                .get_physical_device_surface_present_modes(base.physical_device, self.surface)?;

            Ok(
                if available_formats.contains(&vk::PresentModeKHR::MAILBOX) {
                    vk::PresentModeKHR::MAILBOX
                } else {
                    vk::PresentModeKHR::FIFO
                },
            )
        }
    }

    pub fn choose_swapchain_extent(
        &self,
        base: &VulkanBase,
        width: u32,
        height: u32,
    ) -> VkResult<vk::Extent2D> {
        let capabilities = self.get_capabilities(base)?;
        if capabilities.current_extent.width != u32::MAX {
            Ok(capabilities.current_extent)
        } else {
            use num::clamp;

            Ok(vk::Extent2D {
                width: clamp(
                    width,
                    capabilities.min_image_extent.width,
                    capabilities.max_image_extent.width,
                ),
                height: clamp(
                    height,
                    capabilities.min_image_extent.height,
                    capabilities.max_image_extent.height,
                ),
            })
        }
    }

    pub fn get_capabilities(&self, base: &VulkanBase) -> VkResult<vk::SurfaceCapabilitiesKHR> {
        unsafe {
            self.surface_loader
                .get_physical_device_surface_capabilities(base.physical_device, self.surface)
        }
    }
}

impl Drop for VulkanSurface {
    fn drop(&mut self) {
        unsafe {
            self.surface_loader.destroy_surface(self.surface, None);
        }
    }
}
