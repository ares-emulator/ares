use crate::error;
use crate::error::FilterChainError;
use crate::gl::{DrawQuad, FINAL_VBO_DATA, OFFSCREEN_VBO_DATA};
use bytemuck::offset_of;
use glow::HasContext;
use librashader_runtime::quad::{QuadType, VertexInput};

pub struct Gl3DrawQuad {
    vbo: [glow::Buffer; 2],
    vao: glow::VertexArray,
}

impl DrawQuad for Gl3DrawQuad {
    fn new(ctx: &glow::Context) -> error::Result<Self> {
        let vbo;
        let vao;

        unsafe {
            vbo = [
                ctx.create_buffer().map_err(FilterChainError::GlError)?,
                ctx.create_buffer().map_err(FilterChainError::GlError)?,
            ];

            ctx.bind_buffer(glow::ARRAY_BUFFER, Some(vbo[0]));
            ctx.buffer_data_u8_slice(
                glow::ARRAY_BUFFER,
                bytemuck::cast_slice(OFFSCREEN_VBO_DATA),
                glow::STATIC_DRAW,
            );

            ctx.bind_buffer(glow::ARRAY_BUFFER, Some(vbo[1]));

            ctx.buffer_data_u8_slice(
                glow::ARRAY_BUFFER,
                bytemuck::cast_slice(FINAL_VBO_DATA),
                glow::STATIC_DRAW,
            );

            ctx.bind_buffer(glow::ARRAY_BUFFER, None);

            vao = ctx
                .create_vertex_array()
                .map_err(FilterChainError::GlError)?;
        }

        Ok(Self { vbo, vao })
    }

    fn bind_vertices(&self, ctx: &glow::Context, quad_type: QuadType) {
        let buffer_index = match quad_type {
            QuadType::Offscreen => 0,
            QuadType::Final => 1,
        };

        unsafe {
            ctx.bind_vertex_array(Some(self.vao));
            ctx.enable_vertex_attrib_array(0);
            ctx.enable_vertex_attrib_array(1);

            ctx.bind_buffer(glow::ARRAY_BUFFER, Some(self.vbo[buffer_index]));

            ctx.vertex_attrib_pointer_f32(
                0,
                4,
                glow::FLOAT,
                false,
                std::mem::size_of::<VertexInput>() as i32,
                offset_of!(VertexInput, position) as i32,
            );

            ctx.vertex_attrib_pointer_f32(
                1,
                2,
                glow::FLOAT,
                false,
                std::mem::size_of::<VertexInput>() as i32,
                offset_of!(VertexInput, texcoord) as i32,
            );
        }
    }

    fn unbind_vertices(&self, ctx: &glow::Context) {
        unsafe {
            ctx.disable_vertex_attrib_array(0);
            ctx.disable_vertex_attrib_array(1);
            ctx.bind_vertex_array(None);
            ctx.bind_buffer(glow::ARRAY_BUFFER, None);
        }
    }
}

// impl Drop for Gl3DrawQuad {
//     fn drop(&mut self) {
//         unsafe {
//             if let Some(vbo) = self.vbo {
//                 glow::DeleteBuffers(1, &self.vbo[0]);
//             }
//
//             if self.vbo[1] != 0 {
//                 glow::DeleteBuffers(1, &self.vbo[1]);
//             }
//
//             if self.vao != 0 {
//                 glow::DeleteVertexArrays(1, &self.vao)
//             }
//         }
//     }
// }
