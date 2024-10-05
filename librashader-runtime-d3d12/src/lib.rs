#![cfg(target_os = "windows")]
#![deny(unsafe_op_in_unsafe_fn)]
#![cfg_attr(not(feature = "stable"), feature(type_alias_impl_trait))]

mod buffer;
mod descriptor_heap;
mod draw_quad;
mod filter_chain;
mod filter_pass;
mod framebuffer;
mod graphics_pipeline;
mod luts;
mod mipmap;
mod samplers;
mod texture;
mod util;

pub mod error;
pub mod options;
mod resource;

use librashader_runtime::impl_filter_chain_parameters;
impl_filter_chain_parameters!(FilterChainD3D12);
pub use filter_chain::FilterChainD3D12;
pub use texture::D3D12InputImage;
pub use texture::D3D12OutputView;
