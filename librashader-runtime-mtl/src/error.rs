//! Metal shader runtime errors.
use librashader_common::{FilterMode, WrapMode};
use librashader_preprocess::PreprocessError;
use librashader_presets::ParsePresetError;
use librashader_reflect::error::{ShaderCompileError, ShaderReflectError};
use librashader_runtime::image::ImageError;
use objc2::rc::Retained;
use objc2_foundation::NSError;
use thiserror::Error;

/// Cumulative error type for Metal filter chains.
#[derive(Error, Debug)]
#[non_exhaustive]
pub enum FilterChainError {
    #[error("shader preset parse error")]
    ShaderPresetError(#[from] ParsePresetError),
    #[error("shader preprocess error")]
    ShaderPreprocessError(#[from] PreprocessError),
    #[error("shader compile error")]
    ShaderCompileError(#[from] ShaderCompileError),
    #[error("shader reflect error")]
    ShaderReflectError(#[from] ShaderReflectError),
    #[error("lut loading error")]
    LutLoadError(#[from] ImageError),
    #[error("sampler create error")]
    SamplerError(WrapMode, FilterMode, FilterMode),
    #[error("buffer creation error")]
    BufferError,
    #[error("metal error")]
    MetalError(#[from] Retained<NSError>),
    #[error("couldn't find entry for shader")]
    ShaderWrongEntryName,
    #[error("couldn't create render pass")]
    FailedToCreateRenderPass,
    #[error("couldn't create texture")]
    FailedToCreateTexture,
    #[error("couldn't create command buffer")]
    FailedToCreateCommandBuffer,
    #[error("unreachable")]
    Infallible(#[from] std::convert::Infallible),
}

/// Result type for Metal filter chains.
pub type Result<T> = std::result::Result<T, FilterChainError>;
