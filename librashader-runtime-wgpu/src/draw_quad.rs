use array_concat::concat_arrays;
use bytemuck::{Pod, Zeroable};
use librashader_runtime::quad::QuadType;
use wgpu::util::{BufferInitDescriptor, DeviceExt};
use wgpu::{Buffer, Device, RenderPass};

// As per https://www.w3.org/TR/webgpu/#vertex-processing,
// WGPU does vertex expansion
#[repr(C)]
#[derive(Debug, Copy, Clone, Default, Zeroable, Pod)]
struct WgpuVertex {
    position: [f32; 2],
    texcoord: [f32; 2],
}

const OFFSCREEN_VBO_DATA: [WgpuVertex; 4] = [
    WgpuVertex {
        position: [-1.0, -1.0],
        texcoord: [0.0, 0.0],
    },
    WgpuVertex {
        position: [-1.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    WgpuVertex {
        position: [1.0, -1.0],
        texcoord: [1.0, 0.0],
    },
    WgpuVertex {
        position: [1.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

const FINAL_VBO_DATA: [WgpuVertex; 4] = [
    WgpuVertex {
        position: [0.0, 0.0],
        texcoord: [0.0, 0.0],
    },
    WgpuVertex {
        position: [0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    WgpuVertex {
        position: [1.0, 0.0],
        texcoord: [1.0, 0.0],
    },
    WgpuVertex {
        position: [1.0, 1.0],
        texcoord: [1.0, 1.0],
    },
];

static VBO_DATA: &[WgpuVertex; 8] = &concat_arrays!(OFFSCREEN_VBO_DATA, FINAL_VBO_DATA);

pub struct DrawQuad {
    buffer: Buffer,
}

impl DrawQuad {
    pub fn new(device: &Device) -> DrawQuad {
        let buffer = device.create_buffer_init(&BufferInitDescriptor {
            label: Some("librashader vbo"),
            contents: bytemuck::cast_slice(VBO_DATA),
            usage: wgpu::BufferUsages::VERTEX,
        });

        DrawQuad { buffer }
    }

    pub fn draw_quad<'a, 'b: 'a>(&'b self, cmd: &mut RenderPass<'a>, vbo: QuadType) {
        cmd.set_vertex_buffer(0, self.buffer.slice(0..));

        let offset = match vbo {
            QuadType::Offscreen => 0..4,
            QuadType::Final => 4..8,
        };

        cmd.draw(offset, 0..1)
    }
}
