//! Shader preset definition parsing for librashader.
//!
//! This crate contains facilities and types for parsing `.slangp` shader presets files.
//!
//! Shader presets contain shader and texture parameters, and the order in which to apply a set of
//! shaders in a filter chain. A librashader runtime takes a resulting [`ShaderPreset`](crate::ShaderPreset)
//! as input to create a filter chain.
//!
//! Re-exported as [`librashader::presets`](https://docs.rs/librashader/latest/librashader/presets/index.html).

#![allow(stable_features)]
#![allow(unstable_name_collisions)]

pub mod context;
mod error;
mod extract_if;
mod parse;
mod preset;

pub use context::WildcardContext;
pub use error::*;
pub use preset::*;
