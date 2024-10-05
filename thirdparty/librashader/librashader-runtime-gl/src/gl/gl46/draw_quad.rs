use crate::error;
use crate::error::FilterChainError;
use crate::gl::DrawQuad;
use crate::gl::{FINAL_VBO_DATA, OFFSCREEN_VBO_DATA};
use bytemuck::offset_of;
use glow::HasContext;
use librashader_runtime::quad::{QuadType, VertexInput};

pub struct Gl46DrawQuad {
    vbo: [glow::Buffer; 2],
    vao: glow::VertexArray,
}

impl DrawQuad for Gl46DrawQuad {
    fn new(context: &glow::Context) -> error::Result<Self> {
        let vbo;
        let vao;

        unsafe {
            vbo = [
                context
                    .create_named_buffer()
                    .map_err(FilterChainError::GlError)?,
                context
                    .create_named_buffer()
                    .map_err(FilterChainError::GlError)?,
            ];

            context.named_buffer_data_u8_slice(
                vbo[0],
                bytemuck::cast_slice(OFFSCREEN_VBO_DATA),
                glow::STATIC_DRAW,
            );

            context.named_buffer_data_u8_slice(
                vbo[1],
                bytemuck::cast_slice(FINAL_VBO_DATA),
                glow::STATIC_DRAW,
            );

            vao = context
                .create_named_vertex_array()
                .map_err(FilterChainError::GlError)?;
            context.enable_vertex_array_attrib(vao, 0);
            context.enable_vertex_array_attrib(vao, 1);

            context.vertex_array_attrib_format_f32(
                vao,
                0,
                4,
                glow::FLOAT,
                false,
                offset_of!(VertexInput, position) as u32,
            );
            context.vertex_array_attrib_format_f32(
                vao,
                1,
                2,
                glow::FLOAT,
                false,
                offset_of!(VertexInput, texcoord) as u32,
            );

            context.vertex_array_attrib_binding_f32(vao, 0, 0);
            context.vertex_array_attrib_binding_f32(vao, 1, 0);
        }

        Ok(Self { vbo, vao })
    }

    fn bind_vertices(&self, context: &glow::Context, quad_type: QuadType) {
        let buffer_index = match quad_type {
            QuadType::Offscreen => 0,
            QuadType::Final => 1,
        };

        unsafe {
            context.vertex_array_vertex_buffer(
                self.vao,
                0,
                Some(self.vbo[buffer_index]),
                0,
                std::mem::size_of::<VertexInput>() as i32,
            );

            context.bind_vertex_array(Some(self.vao))
        }
    }

    fn unbind_vertices(&self, context: &glow::Context) {
        unsafe {
            context.bind_vertex_array(None);
        }
    }
}
