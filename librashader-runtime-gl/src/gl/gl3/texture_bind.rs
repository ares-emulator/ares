use crate::gl::BindTexture;
use crate::samplers::SamplerSet;
use crate::texture::InputTexture;
use glow::HasContext;
use librashader_reflect::reflect::semantics::TextureBinding;

pub struct Gl3BindTexture;

impl BindTexture for Gl3BindTexture {
    fn bind_texture(
        ctx: &glow::Context,
        samplers: &SamplerSet,
        binding: &TextureBinding,
        texture: &InputTexture,
    ) {
        unsafe {
            // eprintln!("setting {} to texunit {}", texture.image.handle, binding.binding);;
            ctx.active_texture(glow::TEXTURE0 + binding.binding);
            ctx.bind_texture(glow::TEXTURE_2D, texture.image.handle);
            ctx.bind_sampler(
                binding.binding,
                Some(samplers.get(texture.wrap_mode, texture.filter, texture.mip_filter)),
            );
        }
    }

    fn gen_mipmaps(ctx: &glow::Context, texture: &InputTexture) {
        unsafe {
            ctx.bind_texture(glow::TEXTURE_2D, texture.image.handle);
            ctx.generate_mipmap(glow::TEXTURE_2D);
            ctx.bind_texture(glow::TEXTURE_2D, None);
        }
    }
}
