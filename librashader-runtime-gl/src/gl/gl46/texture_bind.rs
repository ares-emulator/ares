use crate::gl::BindTexture;
use crate::samplers::SamplerSet;
use crate::texture::InputTexture;
use glow::HasContext;
use librashader_reflect::reflect::semantics::TextureBinding;

pub struct Gl46BindTexture;

impl BindTexture for Gl46BindTexture {
    fn bind_texture(
        context: &glow::Context,
        samplers: &SamplerSet,
        binding: &TextureBinding,
        texture: &InputTexture,
    ) {
        unsafe {
            // eprintln!("setting {} to texunit {}", texture.image.handle, binding.binding);
            context.bind_texture_unit(binding.binding, texture.image.handle);
            context.bind_sampler(
                binding.binding,
                Some(samplers.get(texture.wrap_mode, texture.filter, texture.mip_filter)),
            )
        }
    }

    fn gen_mipmaps(context: &glow::Context, texture: &InputTexture) {
        if let Some(texture) = texture.image.handle {
            unsafe { context.generate_texture_mipmap(texture) }
        }
    }
}
