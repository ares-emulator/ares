use crate::buffer::D3D12Buffer;
use crate::error;
use array_concat::concat_arrays;
use bytemuck::{offset_of, Pod, Zeroable};
use librashader_runtime::quad::QuadType;
use windows::core::PCSTR;
use windows::Win32::Graphics::Direct3D::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, ID3D12GraphicsCommandList4, ID3D12Resource,
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, D3D12_INPUT_ELEMENT_DESC, D3D12_VERTEX_BUFFER_VIEW,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R32G32_FLOAT;

#[repr(C)]
#[derive(Debug, Copy, Clone, Default, Zeroable, Pod)]
struct D3D12Vertex {
    position: [f32; 2],
    texcoord: [f32; 2],
    color: [f32; 4],
}

const CLEAR: [f32; 4] = [1.0, 1.0, 1.0, 1.0];

const OFFSCREEN_VBO_DATA: [D3D12Vertex; 4] = [
    D3D12Vertex {
        position: [-1.0, -1.0],
        texcoord: [0.0, 1.0],
        color: CLEAR,
    },
    D3D12Vertex {
        position: [-1.0, 1.0],
        texcoord: [0.0, 0.0],
        color: CLEAR,
    },
    D3D12Vertex {
        position: [1.0, -1.0],
        texcoord: [1.0, 1.0],
        color: CLEAR,
    },
    D3D12Vertex {
        position: [1.0, 1.0],
        texcoord: [1.0, 0.0],
        color: CLEAR,
    },
];

const FINAL_VBO_DATA: [D3D12Vertex; 4] = [
    D3D12Vertex {
        position: [0.0, 0.0],
        texcoord: [0.0, 1.0],
        color: CLEAR,
    },
    D3D12Vertex {
        position: [0.0, 1.0],
        texcoord: [0.0, 0.0],
        color: CLEAR,
    },
    D3D12Vertex {
        position: [1.0, 0.0],
        texcoord: [1.0, 1.0],
        color: CLEAR,
    },
    D3D12Vertex {
        position: [1.0, 1.0],
        texcoord: [1.0, 0.0],
        color: CLEAR,
    },
];

static VBO_DATA: &[D3D12Vertex; 8] = &concat_arrays!(OFFSCREEN_VBO_DATA, FINAL_VBO_DATA);

pub(crate) struct DrawQuad {
    _buffer: ID3D12Resource,
    view: D3D12_VERTEX_BUFFER_VIEW,
}

impl DrawQuad {
    pub fn new(device: &ID3D12Device) -> error::Result<DrawQuad> {
        let stride = std::mem::size_of::<D3D12Vertex>() as u32;
        let size = 2 * std::mem::size_of::<[D3D12Vertex; 4]>() as u32;
        let mut buffer = D3D12Buffer::new(device, size as usize)?;
        buffer
            .map(None)?
            .slice
            .copy_from_slice(bytemuck::cast_slice(VBO_DATA));

        let view = D3D12_VERTEX_BUFFER_VIEW {
            BufferLocation: buffer.gpu_address(),
            SizeInBytes: size,
            StrideInBytes: stride,
        };

        let buffer = buffer.into_raw();
        Ok(DrawQuad {
            _buffer: buffer,
            view,
        })
    }

    pub fn bind_vertices_for_frame(&self, cmd: &ID3D12GraphicsCommandList) {
        unsafe {
            cmd.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            cmd.IASetVertexBuffers(0, Some(&[self.view]));
        }
    }

    // frame uses ID3D12GraphicsCommandList4 for renderpasses, don't need to bother with the cast.
    pub fn draw_quad(&self, cmd: &ID3D12GraphicsCommandList4, vbo_type: QuadType) {
        let offset = match vbo_type {
            QuadType::Offscreen => 0,
            QuadType::Final => 4,
        };

        unsafe { cmd.DrawInstanced(4, 1, offset, 0) }
    }

    pub fn get_spirv_cross_vbo_desc() -> [D3D12_INPUT_ELEMENT_DESC; 2] {
        [
            D3D12_INPUT_ELEMENT_DESC {
                SemanticName: PCSTR(b"TEXCOORD\0".as_ptr()),
                SemanticIndex: 0,
                Format: DXGI_FORMAT_R32G32_FLOAT,
                InputSlot: 0,
                AlignedByteOffset: offset_of!(D3D12Vertex, position) as u32,
                InputSlotClass: D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
            D3D12_INPUT_ELEMENT_DESC {
                SemanticName: PCSTR(b"TEXCOORD\0".as_ptr()),
                SemanticIndex: 1,
                Format: DXGI_FORMAT_R32G32_FLOAT,
                InputSlot: 0,
                AlignedByteOffset: offset_of!(D3D12Vertex, texcoord) as u32,
                InputSlotClass: D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
        ]
    }
}
