use array_concat::concat_arrays;
use librashader_runtime::quad::{QuadType, VertexInput};
use objc2::rc::Retained;
use objc2::runtime::ProtocolObject;
use objc2_metal::{
    MTLBuffer, MTLDevice, MTLPrimitiveType, MTLRenderCommandEncoder, MTLResourceOptions,
};
use std::ffi::c_void;
use std::ptr::NonNull;

use crate::error::{FilterChainError, Result};
use crate::graphics_pipeline::VERTEX_BUFFER_INDEX;

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

const VBO_DATA: [VertexInput; 8] = concat_arrays!(OFFSCREEN_VBO_DATA, FINAL_VBO_DATA);

pub struct DrawQuad {
    buffer: Retained<ProtocolObject<dyn MTLBuffer>>,
}

impl DrawQuad {
    pub fn new(device: &ProtocolObject<dyn MTLDevice>) -> Result<DrawQuad> {
        let vbo_data: &'static [u8] = bytemuck::cast_slice(&VBO_DATA);
        let buffer = unsafe {
            device
                .newBufferWithBytes_length_options(
                    // SAFETY: this pointer is const.
                    // https://developer.apple.com/documentation/metal/mtldevice/1433429-newbufferwithbytes
                    NonNull::new_unchecked(vbo_data.as_ptr() as *mut c_void),
                    vbo_data.len(),
                    if cfg!(target_os = "ios") {
                        MTLResourceOptions::MTLResourceStorageModeShared
                    } else {
                        MTLResourceOptions::MTLResourceStorageModeManaged
                    },
                )
                .ok_or(FilterChainError::BufferError)?
        };

        Ok(DrawQuad { buffer })
    }

    pub fn draw_quad(&self, cmd: &ProtocolObject<dyn MTLRenderCommandEncoder>, vbo: QuadType) {
        // TODO: need to see how naga outputs MSL
        let offset = match vbo {
            QuadType::Offscreen => 0,
            QuadType::Final => 4,
        };

        unsafe {
            cmd.setVertexBuffer_offset_atIndex(Some(&self.buffer), 0, VERTEX_BUFFER_INDEX);
            cmd.drawPrimitives_vertexStart_vertexCount(MTLPrimitiveType::TriangleStrip, offset, 4);
        }
    }
}
