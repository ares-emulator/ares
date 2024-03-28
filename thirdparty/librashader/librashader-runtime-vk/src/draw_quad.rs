use crate::error;
use crate::memory::VulkanBuffer;
use array_concat::concat_arrays;
use ash::vk;
use gpu_allocator::vulkan::Allocator;
use librashader_runtime::quad::{QuadType, VertexInput};
use parking_lot::Mutex;
use std::sync::Arc;

const OFFSCREEN_VBO_DATA: [VertexInput; 4] = [
    VertexInput {
        position: [-1.0, -1.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    VertexInput {
        position: [-1.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VertexInput {
        position: [1.0, -1.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
    VertexInput {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

const FINAL_VBO_DATA: [VertexInput; 4] = [
    VertexInput {
        position: [0.0, 0.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    VertexInput {
        position: [0.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VertexInput {
        position: [1.0, 0.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
    VertexInput {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

static VBO_DATA: &[VertexInput; 8] = &concat_arrays!(OFFSCREEN_VBO_DATA, FINAL_VBO_DATA);

pub struct DrawQuad {
    buffer: VulkanBuffer,
}

impl DrawQuad {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<Mutex<Allocator>>,
    ) -> error::Result<DrawQuad> {
        let mut buffer = VulkanBuffer::new(
            device,
            allocator,
            vk::BufferUsageFlags::VERTEX_BUFFER,
            std::mem::size_of::<[VertexInput; 8]>(),
        )?;

        {
            let slice = buffer.as_mut_slice()?;
            slice.copy_from_slice(bytemuck::cast_slice(VBO_DATA));
        }

        Ok(DrawQuad { buffer })
    }

    pub fn bind_vbo_for_frame(&self, device: &ash::Device, cmd: vk::CommandBuffer) {
        unsafe {
            device.cmd_bind_vertex_buffers(cmd, 0, &[self.buffer.handle], &[0 as vk::DeviceSize])
        }
    }

    pub fn draw_quad(&self, device: &ash::Device, cmd: vk::CommandBuffer, vbo: QuadType) {
        let offset = match vbo {
            QuadType::Offscreen => 0,
            QuadType::Final => 4,
        };

        unsafe {
            device.cmd_draw(cmd, 4, 1, offset, 0);
        }
    }
}
