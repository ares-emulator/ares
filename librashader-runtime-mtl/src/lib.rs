//! librashader Metal runtime
//!
//! This crate should not be used directly.
//! See [`librashader::runtime::mtl`](https://docs.rs/librashader/latest/aarch64-apple-darwin/librashader/runtime/mtl/index.html) instead.

#![cfg(target_vendor = "apple")]
#![cfg_attr(not(feature = "stable"), feature(type_alias_impl_trait))]

mod buffer;
mod draw_quad;
mod filter_chain;
mod filter_pass;
mod graphics_pipeline;
mod luts;
mod samplers;
mod texture;

pub use filter_chain::FilterChainMetal;
use objc2_metal::MTLPixelFormat;

pub mod error;
pub mod options;
use librashader_runtime::impl_filter_chain_parameters;
impl_filter_chain_parameters!(FilterChainMetal);

pub use texture::MetalTextureRef;

fn select_optimal_pixel_format(format: MTLPixelFormat) -> MTLPixelFormat {
    if format == MTLPixelFormat::RGBA8Unorm {
        return MTLPixelFormat::BGRA8Unorm;
    }

    if format == MTLPixelFormat::RGBA8Unorm_sRGB {
        return MTLPixelFormat::BGRA8Unorm_sRGB;
    }
    format
}
