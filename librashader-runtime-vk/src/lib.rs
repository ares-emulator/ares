//! librashader Vulkan runtime
//!
//! This crate should not be used directly.
//! See [`librashader::runtime::vk`](https://docs.rs/librashader/latest/librashader/runtime/vk/index.html) instead.
#![deny(unsafe_op_in_unsafe_fn)]
#![feature(type_alias_impl_trait)]
#![feature(let_chains)]

mod draw_quad;
mod filter_chain;
mod filter_pass;
mod framebuffer;
mod graphics_pipeline;
mod luts;
mod memory;
mod queue_selection;
mod samplers;
mod texture;
mod util;

pub use filter_chain::FilterChainVulkan;
pub use filter_chain::VulkanInstance;
pub use filter_chain::VulkanObjects;
pub use texture::VulkanImage;

use librashader_runtime::impl_filter_chain_parameters;
impl_filter_chain_parameters!(FilterChainVulkan);

pub mod error;
pub mod options;
mod render_pass;
