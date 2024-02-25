use crate::hello_triangle::memory::VulkanImageMemory;
use crate::hello_triangle::surface::VulkanSurface;
use crate::hello_triangle::vulkan_base::VulkanBase;
use ash::prelude::VkResult;
use ash::vk;
use ash::vk::Extent3D;

use std::sync::Arc;

pub struct VulkanSwapchain {
    pub swapchain: vk::SwapchainKHR,
    pub loader: ash::extensions::khr::Swapchain,
    pub format: vk::SurfaceFormatKHR,
    pub extent: vk::Extent2D,
    pub mode: vk::PresentModeKHR,
    pub swapchain_images: Vec<vk::Image>,
    pub swapchain_image_views: Vec<vk::ImageView>,

    pub render_images: Vec<(vk::Image, VulkanImageMemory)>,
    pub render_image_views: Vec<vk::ImageView>,
    device: Arc<ash::Device>,
}

impl VulkanSwapchain {
    pub fn new(
        base: &VulkanBase,
        surface: &VulkanSurface,
        width: u32,
        height: u32,
    ) -> VkResult<VulkanSwapchain> {
        let format = surface.choose_format(base)?;
        let mode = surface.choose_present_mode(base)?;
        let extent = surface.choose_swapchain_extent(base, width, height)?;
        let capabilities = surface.get_capabilities(base)?;

        let image_count = capabilities.min_image_count + 1;
        let image_count = if capabilities.max_image_count > 0 {
            image_count.min(capabilities.max_image_count)
        } else {
            image_count
        };

        if base.graphics_queue != surface.present_queue {
            panic!("exclusive mode only")
        }

        let create_info = vk::SwapchainCreateInfoKHR::builder()
            .surface(surface.surface)
            .present_mode(mode)
            .min_image_count(image_count)
            .image_color_space(format.color_space)
            .image_format(format.format)
            .image_extent(extent)
            .image_sharing_mode(vk::SharingMode::EXCLUSIVE)
            .pre_transform(capabilities.current_transform)
            .composite_alpha(vk::CompositeAlphaFlagsKHR::OPAQUE)
            .clipped(true)
            .image_array_layers(1)
            // todo: switch to IMAGE_USAGE_TRANSFER_DST
            .image_usage(vk::ImageUsageFlags::COLOR_ATTACHMENT);

        let loader = ash::extensions::khr::Swapchain::new(&base.instance, &base.device);

        let swapchain = unsafe { loader.create_swapchain(&create_info, None)? };

        let swapchain_images = unsafe { loader.get_swapchain_images(swapchain)? };

        let mut render_images = vec![];

        // create render imaghes
        for _ in 0..swapchain_images.len() {
            let create_info = vk::ImageCreateInfo::builder()
                .extent(Extent3D {
                    width,
                    height,
                    depth: 1,
                })
                .format(format.format)
                .image_type(vk::ImageType::TYPE_2D)
                .sharing_mode(vk::SharingMode::EXCLUSIVE)
                .samples(vk::SampleCountFlags::TYPE_1)
                .tiling(vk::ImageTiling::OPTIMAL)
                .array_layers(1)
                .mip_levels(1)
                .usage(
                    vk::ImageUsageFlags::SAMPLED
                        | vk::ImageUsageFlags::COLOR_ATTACHMENT
                        | vk::ImageUsageFlags::TRANSFER_SRC,
                )
                .initial_layout(vk::ImageLayout::UNDEFINED);

            unsafe {
                let image = base.device.create_image(&create_info, None)?;
                let mem_reqs = base.device.get_image_memory_requirements(image);

                let memory =
                    VulkanImageMemory::new(&base.device, &base.allocator, mem_reqs, &image)
                        .unwrap();

                render_images.push((image, memory))
            }
        }

        let swapchain_image_views: VkResult<Vec<vk::ImageView>> = swapchain_images
            .iter()
            .map(|image| {
                let create_info = vk::ImageViewCreateInfo::builder()
                    .view_type(vk::ImageViewType::TYPE_2D)
                    .format(format.format)
                    .components(vk::ComponentMapping {
                        r: vk::ComponentSwizzle::IDENTITY,
                        g: vk::ComponentSwizzle::IDENTITY,
                        b: vk::ComponentSwizzle::IDENTITY,
                        a: vk::ComponentSwizzle::IDENTITY,
                    })
                    .subresource_range(vk::ImageSubresourceRange {
                        aspect_mask: vk::ImageAspectFlags::COLOR,
                        base_mip_level: 0,
                        level_count: 1,
                        base_array_layer: 0,
                        layer_count: 1,
                    })
                    .image(*image);

                let view = unsafe { base.device.create_image_view(&create_info, None)? };
                // unsafe {
                //     base.debug
                //         .loader
                //         .set_debug_utils_object_name(
                //             base.device.handle(),
                //             &vk::DebugUtilsObjectNameInfoEXT::builder()
                //                 .object_handle(image.as_raw())
                //                 .object_name(CStr::from_bytes_with_nul_unchecked(
                //                     b"SwapchainImage\0",
                //                 ))
                //                 .object_type(vk::ObjectType::IMAGE)
                //                 ,
                //         )
                //         .expect("could not set object name");
                //     base.debug
                //         .loader
                //         .set_debug_utils_object_name(
                //             base.device.handle(),
                //             &vk::DebugUtilsObjectNameInfoEXT::builder()
                //                 .object_handle(view.as_raw())
                //                 .object_name(CStr::from_bytes_with_nul_unchecked(
                //                     b"SwapchainImageView\0",
                //                 ))
                //                 .object_type(vk::ObjectType::IMAGE_VIEW)
                //                 ,
                //         )
                //         .expect("could not set object name");
                // }
                Ok(view)
            })
            .collect();

        let render_image_views: VkResult<Vec<vk::ImageView>> = render_images
            .iter()
            .map(|(image, _)| {
                let create_info = vk::ImageViewCreateInfo::builder()
                    .view_type(vk::ImageViewType::TYPE_2D)
                    .format(format.format)
                    .components(vk::ComponentMapping {
                        r: vk::ComponentSwizzle::IDENTITY,
                        g: vk::ComponentSwizzle::IDENTITY,
                        b: vk::ComponentSwizzle::IDENTITY,
                        a: vk::ComponentSwizzle::IDENTITY,
                    })
                    .subresource_range(vk::ImageSubresourceRange {
                        aspect_mask: vk::ImageAspectFlags::COLOR,
                        base_mip_level: 0,
                        level_count: 1,
                        base_array_layer: 0,
                        layer_count: 1,
                    })
                    .image(*image);

                let view = unsafe { base.device.create_image_view(&create_info, None)? };
                // unsafe {
                //     base.debug
                //         .loader
                //         .set_debug_utils_object_name(
                //             base.device.handle(),
                //             &vk::DebugUtilsObjectNameInfoEXT::builder()
                //                 .object_handle(view.as_raw())
                //                 .object_name(CStr::from_bytes_with_nul_unchecked(
                //                     b"RenderImageView\0",
                //                 ))
                //                 .object_type(vk::ObjectType::IMAGE_VIEW)
                //                 ,
                //         )
                //         .expect("could not set object name");
                // }
                Ok(view)
            })
            .collect();
        Ok(VulkanSwapchain {
            swapchain,
            loader,
            format,
            extent,
            mode,
            swapchain_images,
            render_images,
            swapchain_image_views: swapchain_image_views?,
            render_image_views: render_image_views?,
            device: base.device.clone(),
        })
    }
}

impl Drop for VulkanSwapchain {
    fn drop(&mut self) {
        unsafe {
            for view in &self.render_image_views {
                self.device.destroy_image_view(*view, None)
            }
            for (view, _memory) in &self.render_images {
                self.device.destroy_image(*view, None);
            }
            self.loader.destroy_swapchain(self.swapchain, None)
        }
    }
}
