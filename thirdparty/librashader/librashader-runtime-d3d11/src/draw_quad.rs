use crate::error;
use crate::error::assume_d3d11_init;
use array_concat::concat_arrays;
use bytemuck::offset_of;
use librashader_runtime::quad::{QuadType, VertexInput};
use windows::core::PCSTR;
use windows::Win32::Graphics::Direct3D::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11Buffer, ID3D11Device, ID3D11DeviceContext, D3D11_BIND_VERTEX_BUFFER, D3D11_BUFFER_DESC,
    D3D11_INPUT_ELEMENT_DESC, D3D11_INPUT_PER_VERTEX_DATA, D3D11_SUBRESOURCE_DATA,
    D3D11_USAGE_IMMUTABLE,
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
    stride: u32,
    vbo: ID3D11Buffer,
}

impl DrawQuad {
    pub fn new(device: &ID3D11Device) -> error::Result<DrawQuad> {
        unsafe {
            let mut vbo = None;
            device.CreateBuffer(
                &D3D11_BUFFER_DESC {
                    ByteWidth: 2 * std::mem::size_of::<[VertexInput; 4]>() as u32,
                    Usage: D3D11_USAGE_IMMUTABLE,
                    BindFlags: D3D11_BIND_VERTEX_BUFFER.0 as u32,
                    CPUAccessFlags: Default::default(),
                    MiscFlags: Default::default(),
                    StructureByteStride: 0,
                },
                Some(&D3D11_SUBRESOURCE_DATA {
                    pSysMem: VBO_DATA.as_ptr().cast(),
                    SysMemPitch: 0,
                    SysMemSlicePitch: 0,
                }),
                Some(&mut vbo),
            )?;
            assume_d3d11_init!(vbo, "CreateBuffer");

            Ok(DrawQuad {
                vbo,
                stride: std::mem::size_of::<VertexInput>() as u32,
            })
        }
    }

    pub fn bind_vbo_for_frame(&self, context: &ID3D11DeviceContext) {
        unsafe {
            context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

            context.IASetVertexBuffers(
                0,
                1,
                Some(&Some(self.vbo.clone())),
                Some(&self.stride),
                Some(&0),
            );
        }
    }

    pub fn draw_quad(&self, context: &ID3D11DeviceContext, vbo_type: QuadType) {
        let offset = match vbo_type {
            QuadType::Offscreen => 0,
            QuadType::Final => 4,
        };

        unsafe {
            context.Draw(4, offset);
        }
    }

    pub fn get_spirv_cross_vbo_desc() -> [D3D11_INPUT_ELEMENT_DESC; 2] {
        [
            D3D11_INPUT_ELEMENT_DESC {
                SemanticName: PCSTR(b"TEXCOORD\0".as_ptr()),
                SemanticIndex: 0,
                Format: DXGI_FORMAT_R32G32_FLOAT,
                InputSlot: 0,
                AlignedByteOffset: offset_of!(VertexInput, position) as u32,
                InputSlotClass: D3D11_INPUT_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
            D3D11_INPUT_ELEMENT_DESC {
                SemanticName: PCSTR(b"TEXCOORD\0".as_ptr()),
                SemanticIndex: 1,
                Format: DXGI_FORMAT_R32G32_FLOAT,
                InputSlot: 0,
                AlignedByteOffset: offset_of!(VertexInput, texcoord) as u32,
                InputSlotClass: D3D11_INPUT_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
        ]
    }
}
