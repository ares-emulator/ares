use crate::error;
use crate::error::{assume_d3d_init, Result};
use array_concat::concat_arrays;
use bytemuck::offset_of;
use librashader_runtime::quad::{QuadType, VertexInput};

use windows::Win32::Foundation::FALSE;

use windows::Win32::Graphics::Direct3D9::{
    IDirect3DDevice9, IDirect3DVertexBuffer9, IDirect3DVertexDeclaration9, D3DCMP_ALWAYS,
    D3DCULL_NONE, D3DDECLMETHOD_DEFAULT, D3DDECLTYPE_FLOAT2, D3DDECLTYPE_FLOAT3,
    D3DDECLTYPE_UNUSED, D3DDECLUSAGE_TEXCOORD, D3DPOOL_DEFAULT, D3DPT_TRIANGLESTRIP,
    D3DRS_ALPHABLENDENABLE, D3DRS_CLIPPING, D3DRS_COLORWRITEENABLE, D3DRS_CULLMODE, D3DRS_LIGHTING,
    D3DRS_ZENABLE, D3DRS_ZFUNC, D3DTRANSFORMSTATETYPE, D3DTS_PROJECTION, D3DTS_VIEW,
    D3DVERTEXELEMENT9,
};

const OFFSCREEN_VBO_DATA: [VertexInput; 4] = [
    VertexInput {
        position: [-1.0, -1.0, 0.0, 1.0],
        texcoord: [0.0, 1.0],
    },
    VertexInput {
        position: [1.0, -1.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
    VertexInput {
        position: [-1.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
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
        position: [1.0, 0.0, 0.0, 1.0],
        texcoord: [1.0, 1.0],
    },
    VertexInput {
        position: [0.0, 1.0, 0.0, 1.0],
        texcoord: [0.0, 0.0],
    },
    VertexInput {
        position: [1.0, 1.0, 0.0, 1.0],
        texcoord: [1.0, 0.0],
    },
];

static VBO_DATA: &[VertexInput; 8] = &concat_arrays!(OFFSCREEN_VBO_DATA, FINAL_VBO_DATA);

pub(crate) struct DrawQuad {
    vbo: IDirect3DVertexBuffer9,
    vao: IDirect3DVertexDeclaration9,
}

impl DrawQuad {
    pub fn new(device: &IDirect3DDevice9) -> error::Result<DrawQuad> {
        unsafe {
            let mut vbo = None;
            device.CreateVertexBuffer(
                2 * std::mem::size_of::<[VertexInput; 4]>() as u32,
                0,
                0,
                D3DPOOL_DEFAULT,
                &mut vbo,
                std::ptr::null_mut(),
            )?;

            assume_d3d_init!(vbo, "CreateVertexBuffer");

            let mut ptr = std::ptr::null_mut();
            vbo.Lock(
                0,
                2 * std::mem::size_of::<[VertexInput; 4]>() as u32,
                &mut ptr,
                0,
            )?;
            std::ptr::copy_nonoverlapping(VBO_DATA.as_ptr(), ptr.cast::<VertexInput>(), 8);
            vbo.Unlock()?;

            let vao = device.CreateVertexDeclaration(Self::get_spirv_cross_vbo_desc().as_ptr())?;
            Ok(DrawQuad { vbo, vao })
        }
    }

    pub fn draw_quad(
        &self,
        device: &IDirect3DDevice9,
        vbo_type: QuadType,
        mvp: &[f32; 16],
    ) -> Result<()> {
        let offset = match vbo_type {
            QuadType::Offscreen => 0,
            QuadType::Final => 4,
        };

        unsafe {
            device.SetTransform(D3DTS_PROJECTION, mvp.as_ptr().cast())?;
            device.SetTransform(D3DTS_VIEW, mvp.as_ptr().cast())?;
            device.SetTransform(D3DTRANSFORMSTATETYPE(256), mvp.as_ptr().cast())?;

            device.SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE.0 as u32)?;
            device.SetRenderState(D3DRS_CLIPPING, FALSE.0 as u32)?;
            device.SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS.0 as u32)?;
            device.SetRenderState(D3DRS_ZENABLE, FALSE.0 as u32)?;
            device.SetRenderState(D3DRS_LIGHTING, FALSE.0 as u32)?;

            device.SetRenderState(D3DRS_COLORWRITEENABLE, 0xfu32)?;
            device.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE.0 as u32)?;
            device.BeginScene()?;
            device.SetStreamSource(0, &self.vbo, 0, std::mem::size_of::<VertexInput>() as u32)?;
            // device.SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1)?;
            device.SetVertexDeclaration(&self.vao)?;

            device.DrawPrimitive(D3DPT_TRIANGLESTRIP, offset, 2)?;
            device.EndScene()?;
        }

        Ok(())
    }

    pub fn get_spirv_cross_vbo_desc() -> [D3DVERTEXELEMENT9; 3] {
        [
            D3DVERTEXELEMENT9 {
                Stream: 0,
                Offset: offset_of!(VertexInput, position) as u16,
                Type: D3DDECLTYPE_FLOAT3.0 as u8,
                Method: D3DDECLMETHOD_DEFAULT.0 as u8,
                Usage: D3DDECLUSAGE_TEXCOORD.0 as u8,
                UsageIndex: 0,
            },
            D3DVERTEXELEMENT9 {
                Stream: 0,
                Offset: offset_of!(VertexInput, texcoord) as u16,
                Type: D3DDECLTYPE_FLOAT2.0 as u8,
                Method: D3DDECLMETHOD_DEFAULT.0 as u8,
                Usage: D3DDECLUSAGE_TEXCOORD.0 as u8,
                UsageIndex: 1,
            },
            D3DVERTEXELEMENT9 {
                Stream: 0xFF,
                Offset: 0,
                Type: D3DDECLTYPE_UNUSED.0 as u8,
                Method: 0,
                Usage: 0,
                UsageIndex: 0,
            },
        ]
    }
}
