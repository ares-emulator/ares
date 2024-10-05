use crate::binding::UniformLocation;
use crate::error;
use crate::error::FilterChainError;
use crate::gl::UboRing;
use glow::HasContext;
use librashader_reflect::reflect::semantics::BufferReflection;
use librashader_runtime::ringbuffer::InlineRingBuffer;
use librashader_runtime::ringbuffer::RingBuffer;
use librashader_runtime::uniforms::UniformStorageAccess;

pub struct Gl3UboRing<const SIZE: usize> {
    ring: InlineRingBuffer<glow::Buffer, SIZE>,
}

impl<const SIZE: usize> UboRing<SIZE> for Gl3UboRing<SIZE> {
    fn new(ctx: &glow::Context, buffer_size: u32) -> error::Result<Self> {
        let items: [glow::Buffer; SIZE] = array_init::try_array_init(|_| unsafe {
            ctx.create_buffer().map(|buffer| {
                ctx.bind_buffer(glow::UNIFORM_BUFFER, Some(buffer));
                ctx.buffer_data_size(glow::UNIFORM_BUFFER, buffer_size as i32, glow::STREAM_DRAW);
                ctx.bind_buffer(glow::UNIFORM_BUFFER, None);
                buffer
            })
        })
        .map_err(FilterChainError::GlError)?;

        let ring: InlineRingBuffer<glow::Buffer, SIZE> = InlineRingBuffer::from_array(items);
        Ok(Gl3UboRing { ring })
    }

    fn bind_for_frame(
        &mut self,
        ctx: &glow::Context,
        ubo: &BufferReflection<u32>,
        ubo_location: &UniformLocation<Option<u32>>,
        storage: &impl UniformStorageAccess,
    ) {
        let buffer = *self.ring.current();

        unsafe {
            ctx.bind_buffer(glow::UNIFORM_BUFFER, Some(buffer));
            ctx.buffer_sub_data_u8_slice(
                glow::UNIFORM_BUFFER,
                0,
                &storage.ubo_slice()[0..ubo.size as usize],
            );
            ctx.bind_buffer(glow::UNIFORM_BUFFER, None);

            if let Some(vertex) = ubo_location
                .vertex
                .filter(|vertex| *vertex != glow::INVALID_INDEX)
            {
                ctx.bind_buffer_base(glow::UNIFORM_BUFFER, vertex, Some(buffer));
            }
            if let Some(fragment) = ubo_location
                .fragment
                .filter(|fragment| *fragment != glow::INVALID_INDEX)
            {
                ctx.bind_buffer_base(glow::UNIFORM_BUFFER, fragment, Some(buffer));
            }
        }
        self.ring.next()
    }
}
