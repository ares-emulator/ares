use crate::error::FilterChainError;
use crate::mipmap::MipmapGen;
use librashader_common::{FilterMode, ImageFormat, Size, WrapMode};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::{MipmapSize, ScaleFramebuffer, ViewportSize};
use std::sync::Arc;
use wgpu::TextureFormat;

pub struct OwnedImage {
    device: Arc<wgpu::Device>,
    pub image: Arc<wgpu::Texture>,
    pub view: Arc<wgpu::TextureView>,
    pub max_miplevels: u32,
    pub levels: u32,
    pub size: Size<u32>,
}

#[derive(Clone)]
pub struct InputImage {
    /// A handle to the `VkImage`.
    pub image: Arc<wgpu::Texture>,
    pub view: Arc<wgpu::TextureView>,
    pub wrap_mode: WrapMode,
    pub filter_mode: FilterMode,
    pub mip_filter: FilterMode,
}

impl AsRef<InputImage> for InputImage {
    fn as_ref(&self) -> &InputImage {
        &self
    }
}

impl OwnedImage {
    pub fn new(
        device: Arc<wgpu::Device>,
        size: Size<u32>,
        max_miplevels: u32,
        format: TextureFormat,
    ) -> Self {
        let texture = device.create_texture(&wgpu::TextureDescriptor {
            label: None,
            size: size.into(),
            mip_level_count: std::cmp::min(max_miplevels, size.calculate_miplevels()),
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format,
            usage: wgpu::TextureUsages::TEXTURE_BINDING
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::COPY_DST
                | wgpu::TextureUsages::COPY_SRC,
            view_formats: &[format.into()],
        });

        let view = texture.create_view(&wgpu::TextureViewDescriptor {
            label: None,
            format: Some(format),
            dimension: Some(wgpu::TextureViewDimension::D2),
            aspect: wgpu::TextureAspect::All,
            base_mip_level: 0,
            mip_level_count: None,
            base_array_layer: 0,
            array_layer_count: None,
        });

        Self {
            device,
            image: Arc::new(texture),
            view: Arc::new(view),
            max_miplevels,
            levels: std::cmp::min(max_miplevels, size.calculate_miplevels()),
            size,
        }
    }

    pub fn scale(
        &mut self,
        scaling: Scale2D,
        format: TextureFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        mipmap: bool,
    ) -> Size<u32> {
        let size = source_size.scale_viewport(scaling, *viewport_size, *original_size);
        if self.size != size
            || (mipmap && self.max_miplevels == 1)
            || (!mipmap && self.max_miplevels != 1)
            || format != self.image.format()
        {
            let mut new = OwnedImage::new(
                Arc::clone(&self.device),
                size,
                self.max_miplevels,
                format.into(),
            );
            std::mem::swap(self, &mut new);
        }
        size
    }

    pub(crate) fn as_input(&self, filter: FilterMode, wrap_mode: WrapMode) -> InputImage {
        InputImage {
            image: Arc::clone(&self.image),
            view: Arc::clone(&self.view),
            wrap_mode,
            filter_mode: filter,
            mip_filter: filter,
        }
    }

    pub fn copy_from(&mut self, cmd: &mut wgpu::CommandEncoder, source: &wgpu::Texture) {
        let source_size = source.size().into();
        if source.format() != self.image.format() || self.size != source_size {
            let mut new = OwnedImage::new(
                Arc::clone(&self.device),
                source_size,
                self.max_miplevels,
                source.format(),
            );
            std::mem::swap(self, &mut new);
        }

        cmd.copy_texture_to_texture(
            source.as_image_copy(),
            self.image.as_image_copy(),
            source.size(),
        )
    }

    pub fn clear(&self, cmd: &mut wgpu::CommandEncoder) {
        cmd.clear_texture(&self.image, &wgpu::ImageSubresourceRange::default());
    }
    pub fn generate_mipmaps(
        &self,
        cmd: &mut wgpu::CommandEncoder,
        mipmapper: &mut MipmapGen,
        sampler: &wgpu::Sampler,
    ) {
        mipmapper.generate_mipmaps(cmd, &self.image, sampler, self.max_miplevels);
    }
}

impl ScaleFramebuffer for OwnedImage {
    type Error = FilterChainError;
    type Context = ();

    fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        should_mipmap: bool,
        _context: &Self::Context,
    ) -> Result<Size<u32>, Self::Error> {
        let format: Option<wgpu::TextureFormat> = format.into();
        let format = format.unwrap_or(TextureFormat::Bgra8Unorm);
        Ok(self.scale(
            scaling,
            format,
            viewport_size,
            source_size,
            original_size,
            should_mipmap,
        ))
    }
}
