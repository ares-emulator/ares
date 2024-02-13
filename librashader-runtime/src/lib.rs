//! Helpers and shared logic for librashader runtime implementations.
//!
//! Most of this is only useful when _writing_ a librashader runtime implementations,
//! not _using_ a librashader runtime. Types useful for _using_ the runtime implementations
//! will be re-exported in [`librashader::runtime`](https://docs.rs/librashader/latest/librashader/runtime/index.html).
//!
//! If you are _writing_ a librashader runtime implementation, using these traits and helpers will
//! help in maintaining consistent behaviour in binding semantics and image handling.

/// Scaling helpers.
pub mod scaling;

/// Uniform binding helpers.
pub mod uniforms;

/// Parameter reflection helpers and traits.
pub mod parameters;

/// Image handling helpers.
pub mod image;

/// Ringbuffer helpers
pub mod ringbuffer;

/// Generic implementation of semantics binding.
pub mod binding;

/// VBO helper utilities.
pub mod quad;

/// Filter pass helpers and common traits.
pub mod filter_pass;

/// Common types for render targets.
pub mod render_target;

/// Helpers for handling framebuffers.
pub mod framebuffer;

/// array_chunks_mut polyfill
mod array_chunks_mut;
