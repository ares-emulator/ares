pub use image::ImageError;
use librashader_common::Size;
use std::marker::PhantomData;

use crate::array_chunks_mut::ArrayChunksMut;
use std::path::Path;

/// An uncompressed raw image ready to upload to GPU buffers.
pub struct Image<P: PixelFormat = RGBA8> {
    /// The raw bytes of the image.
    pub bytes: Vec<u8>,
    /// The size dimensions of the image.
    pub size: Size<u32>,
    /// The byte pitch of the image.
    pub pitch: usize,
    _pd: PhantomData<P>,
}

/// R8G8B8A8 pixel format.
///
/// Every RGB with alpha pixel is represented with 32 bits.
pub struct RGBA8;

/// B8G8R8A8 pixel format.
///
/// Every BGR with alpha pixel is represented with 32 bits.
pub struct BGRA8;

/// Represents an image pixel format to convert images into.
pub trait PixelFormat {
    #[doc(hidden)]
    fn convert(pixels: &mut Vec<u8>);
}

impl PixelFormat for RGBA8 {
    fn convert(_pixels: &mut Vec<u8>) {}
}

impl PixelFormat for BGRA8 {
    fn convert(pixels: &mut Vec<u8>) {
        assert!(pixels.len() % 4 == 0);
        for [r, _g, b, _a] in ArrayChunksMut::new(pixels) {
            std::mem::swap(b, r)
        }
    }
}

/// The direction of UV coordinates to load the image for.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum UVDirection {
    /// Origin is at the top left (Direct3D, Vulkan)
    TopLeft,
    /// Origin is at the bottom left (OpenGL)
    BottomLeft,
}

impl<P: PixelFormat> Image<P> {
    /// Load the image from the path as RGBA8.
    pub fn load(path: impl AsRef<Path>, direction: UVDirection) -> Result<Self, ImageError> {
        let mut image = image::open(path.as_ref())?;

        if direction == UVDirection::BottomLeft {
            image = image.flipv();
        }

        let image = image.to_rgba8();

        let height = image.height();
        let width = image.width();
        let pitch = image
            .sample_layout()
            .height_stride
            .max(image.sample_layout().width_stride);

        let mut bytes = image.into_raw();
        P::convert(&mut bytes);
        Ok(Image {
            bytes,
            pitch,
            size: Size { height, width },
            _pd: Default::default(),
        })
    }
}
