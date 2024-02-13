//! This crate implements the librashader transparent cache.
//!
//! This crate is exempt from semantic versioning guarantees and is an implementation
//! detail of librashader runtimes.

mod cache;

mod compilation;

mod cacheable;
mod key;

pub use cacheable::Cacheable;
pub use key::CacheKey;

pub use compilation::CachedCompilation;

pub use cache::cache_pipeline;
pub use cache::cache_shader_object;

#[cfg(all(target_os = "windows", feature = "d3d"))]
mod d3d;
