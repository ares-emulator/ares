use crate::buffer::D3D12Buffer;
use crate::error;
use array_concat::concat_arrays;
use bytemuck::offset_of;
use gpu_allocator::d3d12::Allocator;
use librashader_runtime::quad::{QuadType, VertexInput};
use parking_lot::Mutex;
use std::sync::Arc;
use windows::core::PCSTR;
use windows::Win32::Graphics::Direct3D::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12GraphicsCommandList, ID3D12GraphicsCommandList4,
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, D3D12_INPUT_ELEMENT_DESC, D3D12_VERTEX_BUFFER_VIEW,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R32G32_FLOAT;

const OFFSCREEN_VBO_DATA: [VertexInput; 4] = [
    VertexInput {
        position: [-1.0, -1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VertexInput {
        position: [-1.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    VertexInput {
        position: [1.0, -1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
    VertexInput {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
];

const FINAL_VBO_DATA: [VertexInput; 4] = [
    VertexInput {
        position: [0.0, 0.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VertexInput {
        position: [0.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    VertexInput {
        position: [1.0, 0.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
    VertexInput {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
];

static VBO_DATA: &[VertexInput; 8] = &concat_arrays!(OFFSCREEN_VBO_DATA, FINAL_VBO_DATA);

pub(crate) struct DrawQuad {
    _buffer: D3D12Buffer,
    view: D3D12_VERTEX_BUFFER_VIEW,
}

impl DrawQuad {
    pub fn new(allocator: &Arc<Mutex<Allocator>>) -> error::Result<DrawQuad> {
        let stride = std::mem::size_of::<VertexInput>() as u32;
        let size = 2 * std::mem::size_of::<[VertexInput; 4]>() as u32;
        let mut buffer = D3D12Buffer::new(allocator, size as usize)?;
        buffer
            .map(None)?
            .slice
            .copy_from_slice(bytemuck::cast_slice(VBO_DATA));

        let view = D3D12_VERTEX_BUFFER_VIEW {
            BufferLocation: buffer.gpu_address(),
            SizeInBytes: size,
            StrideInBytes: stride,
        };

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
                AlignedByteOffset: offset_of!(VertexInput, position) as u32,
                InputSlotClass: D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
            D3D12_INPUT_ELEMENT_DESC {
                SemanticName: PCSTR(b"TEXCOORD\0".as_ptr()),
                SemanticIndex: 1,
                Format: DXGI_FORMAT_R32G32_FLOAT,
                InputSlot: 0,
                AlignedByteOffset: offset_of!(VertexInput, texcoord) as u32,
                InputSlotClass: D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
        ]
    }
}
