use crate::gl::FINAL_VBO_DATA;
use crate::gl::{DrawQuad, OpenGLVertex};
use gl::types::{GLint, GLsizeiptr, GLuint};
use std::mem::offset_of;
pub struct Gl46DrawQuad {
    vbo: GLuint,
    vao: GLuint,
}

impl DrawQuad for Gl46DrawQuad {
    fn new() -> Self {
        let mut vbo = 0;
        let mut vao = 0;

        unsafe {
            gl::CreateBuffers(1, &mut vbo);
            gl::NamedBufferData(
                vbo,
                4 * std::mem::size_of::<OpenGLVertex>() as GLsizeiptr,
                FINAL_VBO_DATA.as_ptr().cast(),
                gl::STATIC_DRAW,
            );
            gl::CreateVertexArrays(1, &mut vao);

            gl::EnableVertexArrayAttrib(vao, 0);
            gl::EnableVertexArrayAttrib(vao, 1);

            gl::VertexArrayVertexBuffer(
                vao,
                0,
                vbo,
                0,
                std::mem::size_of::<OpenGLVertex>() as GLint,
            );

            gl::VertexArrayAttribFormat(
                vao,
                0,
                2,
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

    fn bind_vertices(&self) {
        unsafe {
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
            if self.vbo != 0 {
                gl::DeleteBuffers(1, &self.vbo);
            }

            if self.vao != 0 {
                gl::DeleteVertexArrays(1, &self.vao)
            }
        }
    }
}
