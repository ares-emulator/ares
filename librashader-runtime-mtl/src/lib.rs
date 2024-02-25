#![cfg(target_vendor = "apple")]
#![feature(type_alias_impl_trait)]

mod buffer;
mod draw_quad;
mod filter_chain;
mod filter_pass;
mod graphics_pipeline;
mod luts;
mod samplers;
mod texture;

pub use filter_chain::FilterChainMetal;
use icrate::Metal::{
    MTLPixelFormat, MTLPixelFormatBGRA8Unorm, MTLPixelFormatBGRA8Unorm_sRGB,
    MTLPixelFormatRGBA8Unorm, MTLPixelFormatRGBA8Unorm_sRGB,
};

pub mod error;
pub mod options;
use librashader_runtime::impl_filter_chain_parameters;
impl_filter_chain_parameters!(FilterChainMetal);

pub use texture::MetalTextureRef;

fn select_optimal_pixel_format(format: MTLPixelFormat) -> MTLPixelFormat {
    if format == MTLPixelFormatRGBA8Unorm {
        return MTLPixelFormatBGRA8Unorm;
    }

    if format == MTLPixelFormatRGBA8Unorm_sRGB {
        return MTLPixelFormatBGRA8Unorm_sRGB;
    }
    return format;
}
