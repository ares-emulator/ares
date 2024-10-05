use crate::error::{FilterChainError, Result};
use crate::texture::InputTexture;
use librashader_presets::TextureMeta;
use librashader_runtime::image::{Image, BGRA8};
use librashader_runtime::scaling::MipmapSize;
use objc2::runtime::ProtocolObject;
use objc2_metal::{
    MTLBlitCommandEncoder, MTLDevice, MTLOrigin, MTLPixelFormat, MTLRegion, MTLSize,
    MTLStorageMode, MTLTexture, MTLTextureDescriptor, MTLTextureUsage,
};
use std::ffi::c_void;
use std::ptr::NonNull;

pub(crate) struct LutTexture(InputTexture);

impl AsRef<InputTexture> for LutTexture {
    fn as_ref(&self) -> &InputTexture {
        self.0.as_ref()
    }
}

impl LutTexture {
    pub fn new(
        device: &ProtocolObject<dyn MTLDevice>,
        image: Image<BGRA8>,
        config: &TextureMeta,
        mipmapper: &ProtocolObject<dyn MTLBlitCommandEncoder>,
    ) -> Result<Self> {
        let descriptor = unsafe {
            let descriptor =
                MTLTextureDescriptor::texture2DDescriptorWithPixelFormat_width_height_mipmapped(
                    MTLPixelFormat::BGRA8Unorm,
                    image.size.width as usize,
                    image.size.height as usize,
                    config.mipmap,
                );

            descriptor.setSampleCount(1);
            descriptor.setMipmapLevelCount(if config.mipmap {
                image.size.calculate_miplevels() as usize
            } else {
                1
            });

            descriptor.setStorageMode(
                if cfg!(all(target_arch = "aarch64", target_vendor = "apple")) {
                    MTLStorageMode::Shared
                } else {
                    MTLStorageMode::Managed
                },
            );

            descriptor.setUsage(MTLTextureUsage::ShaderRead);

            descriptor
        };

        let texture = device
            .newTextureWithDescriptor(&descriptor)
            .ok_or(FilterChainError::FailedToCreateTexture)?;

        unsafe {
            let region = MTLRegion {
                origin: MTLOrigin { x: 0, y: 0, z: 0 },
                size: MTLSize {
                    width: image.size.width as usize,
                    height: image.size.height as usize,
                    depth: 1,
                },
            };

            texture.replaceRegion_mipmapLevel_withBytes_bytesPerRow(
                region,
                0,
                // SAFETY: replaceRegion withBytes is const.
                NonNull::new_unchecked(image.bytes.as_slice().as_ptr() as *mut c_void),
                4 * image.size.width as usize,
            )
        }

        if config.mipmap && texture.mipmapLevelCount() > 1 {
            mipmapper.generateMipmapsForTexture(&texture);
        }

        Ok(LutTexture(InputTexture {
            texture,
            wrap_mode: config.wrap_mode,
            filter_mode: config.filter_mode,
            mip_filter: config.filter_mode,
        }))
    }
}
