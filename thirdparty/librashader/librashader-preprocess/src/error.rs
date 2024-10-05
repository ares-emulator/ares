use librashader_common::map::ShortString;
use std::convert::Infallible;
use std::path::PathBuf;
use thiserror::Error;

/// Error type for source preprocessing.
#[derive(Error, Debug)]
pub enum PreprocessError {
    /// The version header was not found in the source file.
    #[error("the version header was missing")]
    MissingVersionHeader,
    /// An IO error occurred when reading the source file.
    #[error("the file was not found during resolution")]
    IOError(PathBuf, std::io::Error),
    /// A known encoding was not found for the file.
    #[error(
        "a known encoding was not found for the file. supported encodings are UTF-8 and Latin-1"
    )]
    EncodingError(PathBuf),
    /// Unexpected EOF when reading the source file.
    #[error("unexpected end of file")]
    UnexpectedEof,
    /// Unexpected end of line when reading the source file.
    #[error("unexpected end of line")]
    UnexpectedEol(usize),
    /// An error occurred when parsing a pragma statement.
    #[error("error parsing pragma")]
    PragmaParseError(String),
    /// The given pragma was declared multiple times with differing values.
    #[error("duplicate pragma found")]
    DuplicatePragmaError(ShortString),
    /// The image format requested by the shader was unknown or not supported.
    #[error("shader format is unknown or not found")]
    UnknownImageFormat,
    /// The stage declared by the shader source was not `vertex` or `fragment`.
    #[error("stage must be either vertex or fragment")]
    InvalidStage,
}

impl From<Infallible> for PreprocessError {
    fn from(_: Infallible) -> Self {
        unreachable!()
    }
}
