use std::path::PathBuf;
use thiserror::Error;

/// Error type for preset parsing.
#[derive(Error, Debug)]
pub enum ParsePresetError {
    /// An error occurred when tokenizing the preset file.
    #[error("shader preset lexing error")]
    LexerError { offset: usize, row: u32, col: usize },
    /// An error occurred when parsing the preset file.
    #[error("shader preset parse error")]
    ParserError {
        offset: usize,
        row: u32,
        col: usize,
        kind: ParseErrorKind,
    },
    /// The scale type was invalid.
    #[error("invalid scale type")]
    InvalidScaleType(String),
    /// The preset reference depth exceeded 16.
    #[error("exceeded maximum reference depth (16)")]
    ExceededReferenceDepth,
    /// An absolute path could not be found to resolve the shader preset against.
    #[error("shader presets must be resolved against an absolute path")]
    RootPathWasNotAbsolute,
    /// An IO error occurred when reading the shader preset.
    #[error("io error on file {0:?}: {1}")]
    IOError(PathBuf, std::io::Error),
    /// The shader preset did not contain valid UTF-8 bytes.
    #[error("expected utf8 bytes but got invalid utf8")]
    Utf8Error(Vec<u8>),
}

/// The kind of error that may occur in parsing.
#[derive(Debug)]
pub enum ParseErrorKind {
    /// Expected an indexed key (i.e. `shader0`, `shader1`, ...)
    Index(&'static str),
    /// Expected a signed integer.
    Int,
    /// Expected an unsigned integer.
    UnsignedInt,
    /// Expected a float.
    Float,
    /// Expected a boolean.
    Bool,
}
