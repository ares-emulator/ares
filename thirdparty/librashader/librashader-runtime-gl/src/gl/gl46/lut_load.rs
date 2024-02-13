use crate::error::Result;
use crate::framebuffer::GLImage;
use crate::gl::LoadLut;
use crate::texture::InputTexture;
use gl::types::{GLsizei, GLuint};
use librashader_presets::TextureConfig;
use librashader_runtime::image::{Image, ImageError, UVDirection};
use librashader_runtime::scaling::MipmapSize;
use rayon::prelude::*;
use rustc_hash::FxHashMap;

pub struct Gl46LutLoad;
impl LoadLut for Gl46LutLoad {
    fn load_luts(textures: &[TextureConfig]) -> Result<FxHashMap<usize, InputTexture>> {
        let mut luts = FxHashMap::default();
        let pixel_unpack = unsafe {
            let mut binding = 0;
            gl::GetIntegerv(gl::PIXEL_UNPACK_BUFFER_BINDING, &mut binding);
            binding
        };

        unsafe {
            gl::BindBuffer(gl::PIXEL_UNPACK_BUFFER, 0);
        }

        let images = textures
            .par_iter()
            .map(|texture| Image::load(&texture.path, UVDirection::BottomLeft))
            .collect::<std::result::Result<Vec<Image>, ImageError>>()?;

        for (index, (texture, image)) in textures.iter().zip(images).enumerate() {
            let levels = if texture.mipmap {
                image.size.calculate_miplevels()
            } else {
                1u32
            };

            let mut handle = 0;
            unsafe {
                gl::CreateTextures(gl::TEXTURE_2D, 1, &mut handle);

                gl::TextureStorage2D(
                    handle,
                    levels as GLsizei,
                    gl::RGBA8,
                    image.size.width as GLsizei,
                    image.size.height as GLsizei,
                );

                gl::PixelStorei(gl::UNPACK_ROW_LENGTH, 0);
                gl::PixelStorei(gl::UNPACK_ALIGNMENT, 4);

                gl::TextureSubImage2D(
                    handle,
                    0,
                    0,
                    0,
                    image.size.width as GLsizei,
                    image.size.height as GLsizei,
                    gl::RGBA,
                    gl::UNSIGNED_BYTE,
                    image.bytes.as_ptr().cast(),
                );

                let mipmap = levels > 1;
                if mipmap {
                    gl::GenerateTextureMipmap(handle);
                }
            }

            luts.insert(
                index,
                InputTexture {
                    image: GLImage {
                        handle,
                        format: gl::RGBA8,
                        size: image.size,
                    },
                    filter: texture.filter_mode,
                    mip_filter: texture.filter_mode,
                    wrap_mode: texture.wrap_mode,
                },
            );
        }

        unsafe {
            gl::BindBuffer(gl::PIXEL_UNPACK_BUFFER, pixel_unpack as GLuint);
        };
        Ok(luts)
    }
}
