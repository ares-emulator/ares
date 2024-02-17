use crate::gl::{DrawQuad, OpenGLVertex};
use crate::gl::{FINAL_VBO_DATA, OFFSCREEN_VBO_DATA};
use bytemuck::offset_of;
use gl::types::{GLint, GLsizeiptr, GLuint};
use librashader_runtime::quad::QuadType;

pub struct Gl46DrawQuad {
    vbo: [GLuint; 2],
    vao: GLuint,
}

impl DrawQuad for Gl46DrawQuad {
    fn new() -> Self {
        let mut vbo = [0, 0];
        let mut vao = 0;

        unsafe {
            gl::CreateBuffers(2, vbo.as_mut_ptr());
            gl::NamedBufferData(
                vbo[0],
                std::mem::size_of_val(OFFSCREEN_VBO_DATA) as GLsizeiptr,
                OFFSCREEN_VBO_DATA.as_ptr().cast(),
                gl::STATIC_DRAW,
            );

            gl::NamedBufferData(
                vbo[1],
                std::mem::size_of_val(FINAL_VBO_DATA) as GLsizeiptr,
                FINAL_VBO_DATA.as_ptr().cast(),
                gl::STATIC_DRAW,
            );

            gl::CreateVertexArrays(1, &mut vao);

            gl::EnableVertexArrayAttrib(vao, 0);
            gl::EnableVertexArrayAttrib(vao, 1);

            gl::VertexArrayAttribFormat(
                vao,
                0,
                4,
                gl::FLOAT,
                gl::FALSE,
                offset_of!(OpenGLVertex, position) as GLuint,
            );
            gl::VertexArrayAttribFormat(
                vao,
                1,
                2,
                gl::FLOAT,
                gl::FALSE,
                offset_of!(OpenGLVertex, texcoord) as GLuint,
            );

            gl::VertexArrayAttribBinding(vao, 0, 0);
            gl::VertexArrayAttribBinding(vao, 1, 0);
        }

        Self { vbo, vao }
    }

    fn bind_vertices(&self, quad_type: QuadType) {
        let buffer_index = match quad_type {
            QuadType::Offscreen => 0,
            QuadType::Final => 1,
        };

        unsafe {
            gl::VertexArrayVertexBuffer(
                self.vao,
                0,
                self.vbo[buffer_index],
                0,
                std::mem::size_of::<OpenGLVertex>() as GLint,
            );

            gl::BindVertexArray(self.vao);
        }
    }

    fn unbind_vertices(&self) {
        unsafe {
            gl::BindVertexArray(0);
        }
    }
}

impl Drop for Gl46DrawQuad {
    fn drop(&mut self) {
        unsafe {
            if self.vbo[0] != 0 {
                gl::DeleteBuffers(1, &self.vbo[0]);
            }

            if self.vbo[1] != 0 {
                gl::DeleteBuffers(1, &self.vbo[1]);
            }

            if self.vao != 0 {
                gl::DeleteVertexArrays(1, &self.vao)
            }
        }
    }
}
