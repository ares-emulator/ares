#![cfg(test)]
#![allow(unused_variables)]
mod command;
mod debug;
mod framebuffer;
mod memory;
mod physicaldevice;
mod pipeline;
mod surface;
mod swapchain;
mod syncobjects;
mod util;

pub(crate) mod vulkan_base;

use ash::vk;
use command::VulkanCommandPool;
use framebuffer::VulkanFramebuffer;
use librashader_runtime_vk::{FilterChainVulkan, VulkanImage};
use pipeline::VulkanPipeline;
use surface::VulkanSurface;
use swapchain::VulkanSwapchain;
use syncobjects::SyncObjects;
use vulkan_base::VulkanBase;

use librashader_common::Viewport;

use librashader_runtime_vk::options::FrameOptionsVulkan;
use winit::event::{Event, WindowEvent};
use winit::event_loop::{EventLoop, EventLoopBuilder};
use winit::platform::windows::EventLoopBuilderExtWindows;

// Constants
const WINDOW_TITLE: &str = "librashader Vulkan";
const WINDOW_WIDTH: u32 = 800;
const WINDOW_HEIGHT: u32 = 600;
const MAX_FRAMES_IN_FLIGHT: usize = 3;

struct VulkanWindow;

impl VulkanWindow {
    fn init_window(event_loop: &EventLoop<()>) -> winit::window::Window {
        winit::window::WindowBuilder::new()
            .with_title(WINDOW_TITLE)
            .with_inner_size(winit::dpi::LogicalSize::new(WINDOW_WIDTH, WINDOW_HEIGHT))
            .with_resizable(false)
            .build(event_loop)
            .expect("Failed to create window.")
    }

    pub fn main_loop(
        event_loop: EventLoop<()>,
        window: winit::window::Window,
        vulkan: VulkanDraw,
        mut filter_chain: FilterChainVulkan,
    ) {
        let mut counter = 0;
        event_loop
            .run(|event, target| {
                match event {
                    Event::WindowEvent {
                        window_id: _,
                        event,
                    } => match event {
                        WindowEvent::Resized(new_size) => {
                            // On macos the window needs to be redrawn manually after resizing
                            window.request_redraw();
                        }
                        WindowEvent::RedrawRequested => {
                            VulkanWindow::draw_frame(counter, &vulkan, &mut filter_chain);
                            counter += 1;
                        }
                        WindowEvent::CloseRequested => target.exit(),
                        _ => {}
                    },
                    Event::AboutToWait => window.request_redraw(),
                    _ => {}
                }
            })
            .unwrap();
    }

    unsafe fn record_command_buffer(
        vulkan: &VulkanDraw,
        framebuffer: vk::Framebuffer,
        cmd: vk::CommandBuffer,
    ) {
        let clear_values = [vk::ClearValue {
            color: vk::ClearColorValue {
                float32: [0.3, 0.3, 0.5, 0.0],
            },
        }];

        let render_pass_begin = vk::RenderPassBeginInfo::builder()
            .render_pass(vulkan.pipeline.renderpass)
            .framebuffer(framebuffer)
            .render_area(vk::Rect2D {
                extent: vulkan.swapchain.extent,
                ..Default::default()
            })
            .clear_values(&clear_values);

        vulkan.base.device.cmd_begin_render_pass(
            cmd,
            &render_pass_begin,
            vk::SubpassContents::INLINE,
        );

        vulkan.base.device.cmd_bind_pipeline(
            cmd,
            vk::PipelineBindPoint::GRAPHICS,
            vulkan.pipeline.graphic_pipeline,
        );

        vulkan.base.device.cmd_set_viewport(
            cmd,
            0,
            &[vk::Viewport {
                max_depth: 1.0,
                width: vulkan.swapchain.extent.width as f32,
                height: vulkan.swapchain.extent.height as f32,
                ..Default::default()
            }],
        );

        vulkan.base.device.cmd_set_scissor(
            cmd,
            0,
            &[vk::Rect2D {
                offset: Default::default(),
                extent: vulkan.swapchain.extent,
            }],
        );

        vulkan.base.device.cmd_draw(cmd, 3, 1, 0, 0);
        vulkan.base.device.cmd_end_render_pass(cmd);
    }

    fn draw_frame(frame: usize, vulkan: &VulkanDraw, filter: &mut FilterChainVulkan) {
        let index = frame % MAX_FRAMES_IN_FLIGHT;
        let in_flight = [vulkan.sync.in_flight[index]];
        let image_available = [vulkan.sync.image_available[index]];
        let render_finished = [vulkan.sync.render_finished[index]];

        unsafe {
            vulkan
                .base
                .device
                .wait_for_fences(&in_flight, true, u64::MAX)
                .unwrap();
            vulkan.base.device.reset_fences(&in_flight).unwrap();

            let (swapchain_index, _) = vulkan
                .swapchain
                .loader
                .acquire_next_image(
                    vulkan.swapchain.swapchain,
                    u64::MAX,
                    image_available[0],
                    vk::Fence::null(),
                )
                .unwrap();

            let cmd = vulkan.render_command_pool.buffers[index];
            let framebuffer = vulkan.render_framebuffers[swapchain_index as usize].framebuffer;
            let framebuffer_image = vulkan.swapchain.render_images[swapchain_index as usize].0;
            let swapchain_image = vulkan.swapchain.swapchain_images[swapchain_index as usize];

            vulkan
                .base
                .device
                .reset_command_buffer(cmd, vk::CommandBufferResetFlags::empty())
                .expect("could not reset command buffer");

            vulkan
                .base
                .device
                .begin_command_buffer(cmd, &vk::CommandBufferBeginInfo::default())
                .expect("failed to begin command buffer");

            util::vulkan_image_layout_transition_levels(
                &vulkan.base.device,
                cmd,
                framebuffer_image,
                1,
                vk::ImageLayout::UNDEFINED,
                vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                vk::AccessFlags::MEMORY_READ
                    | vk::AccessFlags::MEMORY_WRITE
                    | vk::AccessFlags::HOST_READ
                    | vk::AccessFlags::HOST_WRITE
                    | vk::AccessFlags::COLOR_ATTACHMENT_READ
                    | vk::AccessFlags::COLOR_ATTACHMENT_WRITE
                    | vk::AccessFlags::SHADER_READ,
                vk::AccessFlags::COLOR_ATTACHMENT_WRITE | vk::AccessFlags::COLOR_ATTACHMENT_READ,
                vk::PipelineStageFlags::ALL_COMMANDS,
                vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            );

            Self::record_command_buffer(vulkan, framebuffer, cmd);

            util::vulkan_image_layout_transition_levels(
                &vulkan.base.device,
                cmd,
                framebuffer_image,
                1,
                vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                vk::AccessFlags::MEMORY_READ
                    | vk::AccessFlags::MEMORY_WRITE
                    | vk::AccessFlags::HOST_READ
                    | vk::AccessFlags::HOST_WRITE
                    | vk::AccessFlags::COLOR_ATTACHMENT_READ
                    | vk::AccessFlags::COLOR_ATTACHMENT_WRITE
                    | vk::AccessFlags::SHADER_READ,
                vk::AccessFlags::SHADER_READ,
                vk::PipelineStageFlags::ALL_COMMANDS,
                vk::PipelineStageFlags::FRAGMENT_SHADER,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            );
            //
            // util::vulkan_image_layout_transition_levels(
            //     &vulkan.base.device,
            //     cmd,
            //     framebuffer_image,
            //     1,
            //     vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            //     vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            //     vk::AccessFlags::empty(),
            //     vk::AccessFlags::SHADER_READ,
            //     vk::PipelineStageFlags::ALL_GRAPHICS,
            //     vk::PipelineStageFlags::VERTEX_SHADER,
            //     vk::QUEUE_FAMILY_IGNORED,
            //     vk::QUEUE_FAMILY_IGNORED
            // );

            filter
                .frame(
                    &VulkanImage {
                        size: vulkan.swapchain.extent.into(),
                        image: framebuffer_image,
                        format: vulkan.swapchain.format.format,
                    },
                    &Viewport {
                        x: 0.0,
                        y: 0.0,
                        output: VulkanImage {
                            size: vulkan.swapchain.extent.into(),
                            image: swapchain_image,
                            format: vulkan.swapchain.format.format,
                        },
                        mvp: None,
                    },
                    cmd,
                    frame,
                    Some(&FrameOptionsVulkan {
                        clear_history: frame == 0,
                        frame_direction: 0,
                        current_subframe: 1,
                        rotation: 0,
                        total_subframes: 1,
                    }),
                )
                .unwrap();

            // eprintln!("{:x}", framebuffer_image.as_raw());
            // // todo: output image will remove need for ImageLayout::GENERAL
            // // todo: make `frame` render into swapchain image rather than blit.
            // util::vulkan_image_layout_transition_levels(
            //     &vulkan.base.device,
            //     cmd,
            //     framebuffer_image,
            //     1,
            //     vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            //     vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
            //     vk::AccessFlags::SHADER_READ,
            //     vk::AccessFlags::TRANSFER_READ,
            //     vk::PipelineStageFlags::VERTEX_SHADER,
            //     vk::PipelineStageFlags::TRANSFER,
            //     vk::QUEUE_FAMILY_IGNORED,
            //     vk::QUEUE_FAMILY_IGNORED,
            // );
            //
            // let blit_subresource = vk::ImageSubresourceLayers::builder()
            //     .layer_count(1)
            //     .aspect_mask(vk::ImageAspectFlags::COLOR)
            //     ;
            //
            // let src_offsets = [
            //     vk::Offset3D { x: 0, y: 0, z: 0 },
            //     vk::Offset3D {
            //         x: vulkan.swapchain.extent.width as i32,
            //         y: vulkan.swapchain.extent.height as i32,
            //         z: 1,
            //     },
            // ];
            //
            // let dst_offsets = [
            //     vk::Offset3D { x: 0, y: 0, z: 0 },
            //     vk::Offset3D {
            //         x: vulkan.swapchain.extent.width as i32,
            //         y: vulkan.swapchain.extent.height as i32,
            //         z: 1,
            //     },
            // ];
            // vulkan.base.device.cmd_blit_image(
            //     cmd,
            //     framebuffer_image,
            //     vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
            //     swapchain_image,
            //     vk::ImageLayout::TRANSFER_DST_OPTIMAL,
            //     &[vk::ImageBlit {
            //         src_subresource: blit_subresource,
            //         src_offsets,
            //         dst_subresource: blit_subresource,
            //         dst_offsets,
            //     }],
            //     vk::Filter::LINEAR,
            // );

            util::vulkan_image_layout_transition_levels(
                &vulkan.base.device,
                cmd,
                swapchain_image,
                1,
                vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                vk::ImageLayout::PRESENT_SRC_KHR,
                vk::AccessFlags::empty(),
                vk::AccessFlags::TRANSFER_READ,
                vk::PipelineStageFlags::TRANSFER,
                vk::PipelineStageFlags::TRANSFER,
                vk::QUEUE_FAMILY_IGNORED,
                vk::QUEUE_FAMILY_IGNORED,
            );

            vulkan
                .base
                .device
                .end_command_buffer(cmd)
                .expect("failed to record commandbuffer");

            let stage_mask = [vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT];
            let cmd = [cmd];
            let submit_info = [*vk::SubmitInfo::builder()
                .wait_dst_stage_mask(&stage_mask)
                .wait_semaphores(&image_available)
                .signal_semaphores(&render_finished)
                .command_buffers(&cmd)];

            vulkan
                .base
                .device
                .queue_submit(vulkan.base.graphics_queue, &submit_info, in_flight[0])
                .expect("failed to submit queue");

            let swapchain_index = [swapchain_index];
            let swapchain = [vulkan.swapchain.swapchain];
            let present_info = vk::PresentInfoKHR::builder()
                .wait_semaphores(&render_finished)
                .swapchains(&swapchain)
                .image_indices(&swapchain_index);

            vulkan
                .swapchain
                .loader
                .queue_present(vulkan.base.graphics_queue, &present_info)
                .unwrap();

            // vulkan.base.device.device_wait_idle().unwrap();
            // intermediates.dispose();
        }
    }
}

pub fn find_memorytype_index(
    memory_req: &vk::MemoryRequirements,
    memory_prop: &vk::PhysicalDeviceMemoryProperties,
    flags: vk::MemoryPropertyFlags,
) -> Option<u32> {
    memory_prop.memory_types[..memory_prop.memory_type_count as _]
        .iter()
        .enumerate()
        .find(|(index, memory_type)| {
            (1 << index) & memory_req.memory_type_bits != 0
                && memory_type.property_flags & flags == flags
        })
        .map(|(index, _memory_type)| index as _)
}

pub struct VulkanDraw {
    _surface: VulkanSurface,
    base: VulkanBase,
    pub swapchain: VulkanSwapchain,
    pub pipeline: VulkanPipeline,
    pub swapchain_command_pool: VulkanCommandPool,
    pub render_command_pool: VulkanCommandPool,
    pub render_framebuffers: Vec<VulkanFramebuffer>,
    pub sync: SyncObjects,
}

pub fn main(vulkan: VulkanBase, filter_chain: FilterChainVulkan) {
    let event_loop = EventLoopBuilder::new()
        .with_any_thread(true)
        .with_dpi_aware(true)
        .build()
        .unwrap();

    let window = VulkanWindow::init_window(&event_loop);
    let surface = VulkanSurface::new(&vulkan, &window).unwrap();

    let swapchain = VulkanSwapchain::new(&vulkan, &surface, WINDOW_WIDTH, WINDOW_HEIGHT).unwrap();

    let pipeline = unsafe { VulkanPipeline::new(&vulkan, &swapchain) }.unwrap();

    let mut render_framebuffers = vec![];
    for (_index, image) in swapchain.render_image_views.iter().enumerate() {
        render_framebuffers.push(
            VulkanFramebuffer::new(
                &vulkan.device,
                image,
                &pipeline.renderpass,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
            )
            .unwrap(),
        )
    }

    let swapchain_command_pool =
        VulkanCommandPool::new(&vulkan, MAX_FRAMES_IN_FLIGHT as u32).unwrap();
    let render_command_pool = VulkanCommandPool::new(&vulkan, MAX_FRAMES_IN_FLIGHT as u32).unwrap();

    let sync = SyncObjects::new(&vulkan.device, MAX_FRAMES_IN_FLIGHT).unwrap();

    let vulkan = VulkanDraw {
        _surface: surface,
        swapchain,
        base: vulkan,
        pipeline,
        swapchain_command_pool,
        sync,
        render_command_pool,
        render_framebuffers,
    };

    VulkanWindow::main_loop(event_loop, window, vulkan, filter_chain);
}
