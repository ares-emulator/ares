//! librashader OpenGL runtime
//!
//! This crate should not be used directly.
//! See [`librashader::runtime::gl`](https://docs.rs/librashader/latest/librashader/runtime/gl/index.html) instead.
#![deny(unsafe_op_in_unsafe_fn)]
#![feature(let_chains)]
#![feature(type_alias_impl_trait)]

mod binding;
mod filter_chain;
mod filter_pass;
mod framebuffer;
mod util;

mod gl;
mod samplers;
mod texture;

pub mod error;
pub mod options;

pub use crate::gl::GLFramebuffer;
pub use filter_chain::FilterChainGL;
pub use framebuffer::GLImage;
