#![cfg(target_os = "windows")]
#![deny(unsafe_op_in_unsafe_fn)]
//! librashader Direct3D 11 runtime
//!
//! This crate should not be used directly.
//! See [`librashader::runtime::d3d11`](https://docs.rs/librashader/latest/librashader/runtime/d3d11/index.html) instead.

#![feature(type_alias_impl_trait)]
#![feature(let_chains)]

mod draw_quad;
mod filter_chain;
mod filter_pass;
mod framebuffer;
mod graphics_pipeline;
mod samplers;
mod texture;
mod util;

pub mod error;
mod luts;
pub mod options;

use librashader_runtime::impl_filter_chain_parameters;
impl_filter_chain_parameters!(FilterChainD3D11);

pub use filter_chain::FilterChainD3D11;
pub use texture::D3D11InputView;
pub use texture::D3D11OutputView;
