use gl::types::{GLenum, GLuint};
use librashader_common::Size;

/// A handle to an OpenGL texture with format and size information.
///
/// Generally for use as shader resource inputs.
#[derive(Default, Debug, Copy, Clone)]
pub struct GLImage {
    /// A GLuint to the texture.
    pub handle: GLuint,
    /// The format of the texture.
    pub format: GLenum,
    /// The size of the texture.
    pub size: Size<u32>,
}
