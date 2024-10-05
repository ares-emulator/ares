#![cfg(target_os = "windows")]
#![cfg_attr(not(feature = "stable"), feature(type_alias_impl_trait))]

mod binding;
mod d3dx;
mod draw_quad;
pub mod error;
mod filter_chain;
mod filter_pass;
mod graphics_pipeline;
mod luts;
pub mod options;
mod samplers;
mod texture;
mod util;

use librashader_runtime::impl_filter_chain_parameters;
impl_filter_chain_parameters!(FilterChainD3D9);

pub use crate::filter_chain::FilterChainD3D9;
