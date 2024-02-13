use crate::binding::UniformLocation;
use crate::gl::UboRing;
use gl::types::{GLsizei, GLsizeiptr, GLuint};
use librashader_reflect::reflect::semantics::BufferReflection;
use librashader_runtime::ringbuffer::InlineRingBuffer;
use librashader_runtime::ringbuffer::RingBuffer;
use librashader_runtime::uniforms::UniformStorageAccess;

pub struct Gl46UboRing<const SIZE: usize> {
    ring: InlineRingBuffer<GLuint, SIZE>,
}

impl<const SIZE: usize> UboRing<SIZE> for Gl46UboRing<SIZE> {
    fn new(buffer_size: u32) -> Self {
        let mut ring: InlineRingBuffer<GLuint, SIZE> = InlineRingBuffer::new();
        unsafe {
            gl::CreateBuffers(SIZE as GLsizei, ring.items_mut().as_mut_ptr());
            for buffer in ring.items() {
                gl::NamedBufferData(
                    *buffer,
                    buffer_size as GLsizeiptr,
                    std::ptr::null(),
                    gl::STREAM_DRAW,
                );
            }
        };

        Gl46UboRing { ring }
    }

    fn bind_for_frame(
        &mut self,
        ubo: &BufferReflection<u32>,
        ubo_location: &UniformLocation<GLuint>,
        storage: &impl UniformStorageAccess,
    ) {
        let size = ubo.size;
        let buffer = self.ring.current();

        unsafe {
            gl::NamedBufferSubData(*buffer, 0, size as GLsizeiptr, storage.ubo_pointer().cast());

            if ubo_location.vertex != gl::INVALID_INDEX {
                gl::BindBufferBase(gl::UNIFORM_BUFFER, ubo_location.vertex, *buffer);
            }
            if ubo_location.fragment != gl::INVALID_INDEX {
                gl::BindBufferBase(gl::UNIFORM_BUFFER, ubo_location.fragment, *buffer);
            }
        }
        self.ring.next()
    }
}
