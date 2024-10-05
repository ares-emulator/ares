//! librashader WGPU runtime
//!
//! This crate should not be used directly.
//! See [`librashader::runtime::wgpu`](https://docs.rs/librashader/latest/librashader/runtime/wgpu/index.html) instead.
#![deny(unsafe_op_in_unsafe_fn)]
#![cfg_attr(not(feature = "stable"), feature(type_alias_impl_trait))]

mod buffer;
mod draw_quad;
mod filter_chain;
mod filter_pass;
mod framebuffer;
mod graphics_pipeline;
mod handle;
mod luts;
mod mipmap;
mod samplers;
mod texture;
mod util;

pub use filter_chain::FilterChainWgpu;
pub use framebuffer::WgpuOutputView;

pub mod error;
pub mod options;

use librashader_runtime::impl_filter_chain_parameters;
impl_filter_chain_parameters!(FilterChainWgpu);
