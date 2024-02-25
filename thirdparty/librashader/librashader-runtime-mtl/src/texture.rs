use crate::error::{FilterChainError, Result};
use crate::select_optimal_pixel_format;
use icrate::Metal::{
    MTLBlitCommandEncoder, MTLCommandBuffer, MTLCommandEncoder, MTLDevice, MTLPixelFormat,
    MTLStorageModePrivate, MTLTexture, MTLTextureDescriptor, MTLTextureUsageRenderTarget,
    MTLTextureUsageShaderRead, MTLTextureUsageShaderWrite,
};
use librashader_common::{FilterMode, ImageFormat, Size, WrapMode};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::{MipmapSize, ScaleFramebuffer, ViewportSize};
use objc2::rc::Id;
use objc2::runtime::ProtocolObject;

pub type MetalTexture = Id<ProtocolObject<dyn MTLTexture>>;

/// Alias to an `id<MTLTexture>`.
pub type MetalTextureRef<'a> = &'a ProtocolObject<dyn MTLTexture>;

pub struct OwnedTexture {
    pub(crate) texture: MetalTexture,
    pub(crate) max_miplevels: u32,
    size: Size<u32>,
}

pub struct InputTexture {
    pub texture: MetalTexture,
    pub wrap_mode: WrapMode,
    pub filter_mode: FilterMode,
    pub mip_filter: FilterMode,
}

impl InputTexture {
    pub fn try_clone(&self) -> Result<Self> {
        Ok(Self {
            texture: self
                .texture
                .newTextureViewWithPixelFormat(self.texture.pixelFormat())
                .ok_or(FilterChainError::FailedToCreateTexture)?,
            wrap_mode: self.wrap_mode,
            filter_mode: self.filter_mode,
            mip_filter: self.mip_filter,
        })
    }
}
impl AsRef<InputTexture> for InputTexture {
    fn as_ref(&self) -> &InputTexture {
        &self
    }
}

impl OwnedTexture {
    pub fn new(
        device: &ProtocolObject<dyn MTLDevice>,
        size: Size<u32>,
        max_miplevels: u32,
        format: MTLPixelFormat,
    ) -> Result<Self> {
        let descriptor = unsafe {
            let descriptor =
                MTLTextureDescriptor::texture2DDescriptorWithPixelFormat_width_height_mipmapped(
                    select_optimal_pixel_format(format),
                    size.width as usize,
                    size.height as usize,
                    max_miplevels > 1,
                );

            descriptor.setSampleCount(1);
            descriptor.setMipmapLevelCount(if max_miplevels > 1 {
                size.calculate_miplevels() as usize
            } else {
                1
            });

            descriptor.setStorageMode(MTLStorageModePrivate);
            descriptor.setUsage(
                MTLTextureUsageShaderRead
                    | MTLTextureUsageShaderWrite
                    | MTLTextureUsageRenderTarget,
            );

            descriptor
        };

        Ok(Self {
            texture: device
                .newTextureWithDescriptor(&descriptor)
                .ok_or(FilterChainError::FailedToCreateTexture)?,
            max_miplevels,
            size,
        })
    }

    pub fn scale(
        &mut self,
        device: &ProtocolObject<dyn MTLDevice>,
        scaling: Scale2D,
        format: MTLPixelFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        mipmap: bool,
    ) -> Result<Size<u32>> {
        let size = source_size.scale_viewport(scaling, *viewport_size, *original_size);

        if self.size != size
            || (mipmap && self.max_miplevels == 1)
            || (!mipmap && self.max_miplevels != 1)
            || format != select_optimal_pixel_format(format)
        {
            let mut new = OwnedTexture::new(device, size, self.max_miplevels, format)?;
            std::mem::swap(self, &mut new);
        }
        Ok(size)
    }

    pub(crate) fn as_input(&self, filter: FilterMode, wrap_mode: WrapMode) -> Result<InputTexture> {
        Ok(InputTexture {
            texture: self
                .texture
                .newTextureViewWithPixelFormat(self.texture.pixelFormat())
                .ok_or(FilterChainError::FailedToCreateTexture)?,
            wrap_mode,
            filter_mode: filter,
            mip_filter: filter,
        })
    }

    pub fn copy_from(
        &self,
        encoder: &ProtocolObject<dyn MTLBlitCommandEncoder>,
        other: &ProtocolObject<dyn MTLTexture>,
    ) -> Result<()> {
        unsafe {
            encoder.copyFromTexture_toTexture(other, &self.texture);
        }

        if self.texture.mipmapLevelCount() > 1 {
            encoder.generateMipmapsForTexture(&self.texture);
        }

        Ok(())
    }

    pub fn generate_mipmaps(&self, cmd: &ProtocolObject<dyn MTLCommandBuffer>) -> Result<()> {
        let mipmapper = cmd
            .blitCommandEncoder()
            .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;
        if self.texture.mipmapLevelCount() > 1 {
            mipmapper.generateMipmapsForTexture(&self.texture);
        }
        mipmapper.endEncoding();
        Ok(())
    }
}

impl ScaleFramebuffer for OwnedTexture {
    type Error = FilterChainError;
    type Context = ProtocolObject<dyn MTLDevice>;

    fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        should_mipmap: bool,
        context: &Self::Context,
    ) -> std::result::Result<Size<u32>, Self::Error> {
        Ok(self.scale(
            &context,
            scaling,
            format.into(),
            viewport_size,
            source_size,
            original_size,
            should_mipmap,
        )?)
    }
}

pub(crate) fn get_texture_size(texture: &ProtocolObject<dyn MTLTexture>) -> Size<u32> {
    let height = texture.height();
    let width = texture.width();
    Size {
        height: height as u32,
        width: width as u32,
    }
}
