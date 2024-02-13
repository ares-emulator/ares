use crate::error;
use crate::memory::VulkanBuffer;
use array_concat::concat_arrays;
use ash::vk;
use bytemuck::{Pod, Zeroable};
use gpu_allocator::vulkan::Allocator;
use librashader_runtime::quad::QuadType;
use parking_lot::RwLock;
use std::sync::Arc;

// Vulkan does vertex expansion
#[repr(C)]
#[derive(Debug, Copy, Clone, Default, Zeroable, Pod)]
struct VulkanVertex {
    position: [f32; 2],
    texcoord: [f32; 2],
}

const OFFSCREEN_VBO_DATA: [VulkanVertex; 4] = [
    VulkanVertex {
        position: [-1.0, -1.0],
        texcoord: [0.0, 0.0],
    },
    VulkanVertex {
        position: [-1.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VulkanVertex {
        position: [1.0, -1.0],
        texcoord: [1.0, 0.0],
    },
    VulkanVertex {
        position: [1.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

const FINAL_VBO_DATA: [VulkanVertex; 4] = [
    VulkanVertex {
        position: [0.0, 0.0],
        texcoord: [0.0, 0.0],
    },
    VulkanVertex {
        position: [0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VulkanVertex {
        position: [1.0, 0.0],
        texcoord: [1.0, 0.0],
    },
    VulkanVertex {
        position: [1.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

static VBO_DATA: &[VulkanVertex; 8] = &concat_arrays!(OFFSCREEN_VBO_DATA, FINAL_VBO_DATA);

pub struct DrawQuad {
    buffer: VulkanBuffer,
    device: Arc<ash::Device>,
}

impl DrawQuad {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<RwLock<Allocator>>,
    ) -> error::Result<DrawQuad> {
        let mut buffer = VulkanBuffer::new(
            device,
            allocator,
            vk::BufferUsageFlags::VERTEX_BUFFER,
            std::mem::size_of::<[VulkanVertex; 8]>(),
        )?;

        {
            let slice = buffer.as_mut_slice()?;
            slice.copy_from_slice(bytemuck::cast_slice(VBO_DATA));
        }

        Ok(DrawQuad {
            buffer,
            device: device.clone(),
        })
    }

    pub fn bind_vbo_for_frame(&self, cmd: vk::CommandBuffer) {
        unsafe {
            self.device.cmd_bind_vertex_buffers(
                cmd,
                0,
                &[self.buffer.handle],
                &[0 as vk::DeviceSize],
            )
        }
    }

    pub fn draw_quad(&self, cmd: vk::CommandBuffer, vbo: QuadType) {
        let offset = match vbo {
            QuadType::Offscreen => 0,
            QuadType::Final => 4,
        };

        unsafe {
            self.device.cmd_draw(cmd, 4, 1, offset, 0);
        }
    }
}
