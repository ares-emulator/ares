use crate::gl::BindTexture;
use crate::samplers::SamplerSet;
use crate::texture::InputTexture;
use librashader_reflect::reflect::semantics::TextureBinding;

pub struct Gl3BindTexture;

impl BindTexture for Gl3BindTexture {
    fn bind_texture(samplers: &SamplerSet, binding: &TextureBinding, texture: &InputTexture) {
        unsafe {
            // eprintln!("setting {} to texunit {}", texture.image.handle, binding.binding);
            gl::ActiveTexture(gl::TEXTURE0 + binding.binding);

            gl::BindTexture(gl::TEXTURE_2D, texture.image.handle);
            gl::BindSampler(
                binding.binding,
                samplers.get(texture.wrap_mode, texture.filter, texture.mip_filter),
            );
        }
    }

    fn gen_mipmaps(texture: &InputTexture) {
        unsafe {
            gl::BindTexture(gl::TEXTURE_2D, texture.image.handle);
            gl::GenerateMipmap(gl::TEXTURE_2D);
            gl::BindTexture(gl::TEXTURE_2D, 0);
        }
    }
}
