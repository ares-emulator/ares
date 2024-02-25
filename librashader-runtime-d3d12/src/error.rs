//! Direct3D 12 shader runtime errors.
//!
use thiserror::Error;

/// Cumulative error type for Direct3D12 filter chains.
#[derive(Error, Debug)]
pub enum FilterChainError {
    #[error("invariant assumption about d3d11 did not hold. report this as an issue.")]
    Direct3DOperationError(&'static str),
    #[error("direct3d driver error")]
    Direct3DError(#[from] windows::core::Error),
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
    #[error("heap overflow")]
    DescriptorHeapOverflow(usize),
}

/// Result type for Direct3D 12 filter chains.
pub type Result<T> = std::result::Result<T, FilterChainError>;

macro_rules! assume_d3d12_init {
    ($value:ident, $call:literal) => {
        let $value = $value.ok_or($crate::error::FilterChainError::Direct3DOperationError(
            $call,
        ))?;
    };
    (mut $value:ident, $call:literal) => {
        let mut $value = $value.ok_or($crate::error::FilterChainError::Direct3DOperationError(
            $call,
        ))?;
    };
}

/// Macro for unwrapping result of a D3D function.
pub(crate) use assume_d3d12_init;
use librashader_preprocess::PreprocessError;
use librashader_presets::ParsePresetError;
use librashader_reflect::error::{ShaderCompileError, ShaderReflectError};
use librashader_runtime::image::ImageError;
