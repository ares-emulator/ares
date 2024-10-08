mod context;

use crate::render::gl::context::{GLVersion, GlfwContext};
use crate::render::{CommonFrameOptions, RenderTest};
use anyhow::anyhow;
use glow::{HasContext, PixelPackData, PixelUnpackData};
use image::RgbaImage;
use librashader::presets::ShaderPreset;
use librashader::runtime::gl::{FilterChain, FilterChainOptions, FrameOptions, GLImage};
use librashader::runtime::{FilterChainParameters, RuntimeParameters};
use librashader::runtime::{Size, Viewport};
use librashader_runtime::image::{Image, UVDirection, RGBA8};
use std::path::Path;
use std::sync::Arc;

struct OpenGl {
    context: GlfwContext,
    texture: GLImage,
    image_bytes: Image<RGBA8>,
}

pub struct OpenGl3(OpenGl);
pub struct OpenGl4(OpenGl);

impl RenderTest for OpenGl3 {
    fn new(path: &Path) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        OpenGl3::new(path)
    }

    fn image_size(&self) -> Size<u32> {
        self.0.image_bytes.size
    }

    fn render_with_preset_and_params(
        &mut self,
        preset: ShaderPreset,
        frame_count: usize,
        output_size: Option<Size<u32>>,
        param_setter: Option<&dyn Fn(&RuntimeParameters)>,
        frame_options: Option<CommonFrameOptions>,
    ) -> anyhow::Result<image::RgbaImage> {
        let mut filter_chain = unsafe {
            FilterChain::load_from_preset(
                preset,
                Arc::clone(&self.0.context.gl),
                Some(&FilterChainOptions {
                    glsl_version: 330,
                    use_dsa: false,
                    force_no_mipmaps: false,
                    disable_cache: false,
                }),
            )
        }?;

        if let Some(setter) = param_setter {
            setter(filter_chain.parameters());
        }

        Ok(self.0.render(
            &mut filter_chain,
            frame_count,
            output_size,
            frame_options
                .map(|options| FrameOptions {
                    clear_history: options.clear_history,
                    frame_direction: options.frame_direction,
                    rotation: options.rotation,
                    total_subframes: options.total_subframes,
                    current_subframe: options.current_subframe,
                })
                .as_ref(),
        )?)
    }
}

impl RenderTest for OpenGl4 {
    fn new(path: &Path) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        OpenGl4::new(path)
    }

    fn image_size(&self) -> Size<u32> {
        self.0.image_bytes.size
    }

    fn render_with_preset_and_params(
        &mut self,
        preset: ShaderPreset,
        frame_count: usize,
        output_size: Option<Size<u32>>,
        param_setter: Option<&dyn Fn(&RuntimeParameters)>,
        frame_options: Option<CommonFrameOptions>,
    ) -> anyhow::Result<image::RgbaImage> {
        let mut filter_chain = unsafe {
            FilterChain::load_from_preset(
                preset,
                Arc::clone(&self.0.context.gl),
                Some(&FilterChainOptions {
                    glsl_version: 460,
                    use_dsa: true,
                    force_no_mipmaps: false,
                    disable_cache: true,
                }),
            )
        }?;

        if let Some(setter) = param_setter {
            setter(filter_chain.parameters());
        }

        Ok(self.0.render(
            &mut filter_chain,
            frame_count,
            output_size,
            frame_options
                .map(|options| FrameOptions {
                    clear_history: options.clear_history,
                    frame_direction: options.frame_direction,
                    rotation: options.rotation,
                    total_subframes: options.total_subframes,
                    current_subframe: options.current_subframe,
                })
                .as_ref(),
        )?)
    }
}

impl OpenGl3 {
    pub fn new(image_path: &Path) -> anyhow::Result<Self> {
        Ok(Self(OpenGl::new(image_path, false)?))
    }
}

impl OpenGl4 {
    pub fn new(image_path: &Path) -> anyhow::Result<Self> {
        Ok(Self(OpenGl::new(image_path, true)?))
    }
}

impl OpenGl {
    pub fn new(image_path: &Path, use_dsa: bool) -> anyhow::Result<Self> {
        let image: Image<RGBA8> = Image::load(image_path, UVDirection::TopLeft)?;
        let height = image.size.height;
        let width = image.size.width;
        let version = if use_dsa {
            GLVersion(4, 6)
        } else {
            GLVersion(3, 3)
        };

        let context = GlfwContext::new(version, width, height)?;

        let texture = unsafe {
            let tex = context.gl.create_texture().map_err(|s| anyhow!("{}", s))?;
            context.gl.bind_texture(glow::TEXTURE_2D, Some(tex));
            context.gl.tex_storage_2d(
                glow::TEXTURE_2D,
                1,
                glow::RGBA8,
                image.size.width as i32,
                image.size.height as i32,
            );

            context.gl.pixel_store_i32(glow::UNPACK_ROW_LENGTH, 0);
            context.gl.pixel_store_i32(glow::UNPACK_ALIGNMENT, 4);
            context.gl.bind_buffer(glow::PIXEL_UNPACK_BUFFER, None);

            context.gl.tex_sub_image_2d(
                glow::TEXTURE_2D,
                0,
                0,
                0,
                image.size.width as i32,
                image.size.height as i32,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                PixelUnpackData::Slice(&image.bytes),
            );

            context.gl.bind_texture(glow::TEXTURE_2D, None);
            tex
        };

        Ok(Self {
            context,
            texture: GLImage {
                handle: Some(texture),
                format: glow::RGBA8,
                size: image.size,
            },
            image_bytes: image,
        })
    }

    pub fn render(
        &self,
        chain: &mut FilterChain,
        frame_count: usize,
        output_size: Option<Size<u32>>,
        options: Option<&FrameOptions>,
    ) -> Result<RgbaImage, anyhow::Error> {
        let output_size = output_size.unwrap_or(self.image_bytes.size);

        let render_texture = unsafe {
            let tex = self
                .context
                .gl
                .create_texture()
                .map_err(|s| anyhow!("{}", s))?;
            self.context.gl.bind_texture(glow::TEXTURE_2D, Some(tex));
            self.context.gl.tex_storage_2d(
                glow::TEXTURE_2D,
                1,
                glow::RGBA8,
                output_size.width as i32,
                output_size.height as i32,
            );
            self.context.gl.bind_texture(glow::TEXTURE_2D, None);
            tex
        };

        let output = GLImage {
            handle: Some(render_texture),
            format: glow::RGBA8,
            size: output_size,
        };

        let viewport = Viewport::new_render_target_sized_origin(&output, None)?;
        for frame in 0..=frame_count {
            unsafe {
                chain.frame(&self.texture, &viewport, frame, options)?;
            }
        }

        let mut data = vec![0u8; output_size.width as usize * output_size.height as usize * 4];

        unsafe {
            self.context
                .gl
                .bind_texture(glow::TEXTURE_2D, output.handle);
            self.context.gl.get_tex_image(
                glow::TEXTURE_2D,
                0,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                PixelPackData::Slice(&mut data),
            )
        }
        Ok(
            RgbaImage::from_raw(output_size.width, output_size.height, data)
                .ok_or(anyhow!("failed to create image from slice"))?,
        )
    }
}
