pub use image::ImageError;
use librashader_common::Size;
use std::marker::PhantomData;

use image::error::{LimitError, LimitErrorKind};
use image::DynamicImage;
use librashader_pack::{TextureBuffer, TextureResource};
use librashader_presets::TextureMeta;
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

/// A8R8G8B8 pixel format.
///
/// Every BGR with alpha pixel is represented with 32 bits.
pub struct ARGB8;

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
        const BGRA_SWIZZLE: &[usize; 32] = &generate_swizzle([2, 1, 0, 3]);
        swizzle_pixels(pixels, BGRA_SWIZZLE);
    }
}

impl PixelFormat for ARGB8 {
    fn convert(pixels: &mut Vec<u8>) {
        const ARGB_SWIZZLE: &[usize; 32] = &generate_swizzle([3, 0, 1, 2]);
        swizzle_pixels(pixels, ARGB_SWIZZLE);
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
        let image = image::open(path.as_ref())?;
        Ok(Self::convert(image, direction))
    }

    /// Load te image from a [`TextureBuffer`] from a [`ShaderPresetPack`](librashader_pack::ShaderPresetPack).
    pub fn load_from_buffer(
        buffer: TextureBuffer,
        direction: UVDirection,
    ) -> Result<Self, ImageError> {
        let Some(image) = buffer.into() else {
            return Err(ImageError::Limits(LimitError::from_kind(
                LimitErrorKind::InsufficientMemory,
            )));
        };
        let image = DynamicImage::ImageRgba8(image);
        Ok(Self::convert(image, direction))
    }

    fn convert(mut image: DynamicImage, direction: UVDirection) -> Self {
        if direction == UVDirection::BottomLeft {
            image = image.flipv();
        }

        let image = if let DynamicImage::ImageRgba8(image) = image {
            image
        } else {
            image.to_rgba8()
        };

        let height = image.height();
        let width = image.width();
        let pitch = image
            .sample_layout()
            .height_stride
            .max(image.sample_layout().width_stride);

        let mut bytes = image.into_raw();
        P::convert(&mut bytes);
        Image {
            bytes,
            pitch,
            size: Size { height, width },
            _pd: Default::default(),
        }
    }
}

/// Loaded texture data in the proper pixel format from a [`TextureResource`].
pub struct LoadedTexture<P: PixelFormat = RGBA8> {
    /// The loaded image data
    pub image: Image<P>,
    /// Meta information about the texture
    pub meta: TextureMeta,
}

impl<P: PixelFormat> LoadedTexture<P> {
    /// Load the texture with the given UV direction and subpixel ordering.
    pub fn from_texture(
        texture: TextureResource,
        direction: UVDirection,
    ) -> Result<Self, ImageError> {
        Ok(LoadedTexture {
            meta: texture.meta,
            image: Image::load_from_buffer(texture.data, direction)?,
        })
    }
}

// load-bearing #[inline(always)], without it llvm will not vectorize.
#[inline(always)]
fn swizzle_pixels(pixels: &mut Vec<u8>, swizzle: &'static [usize; 32]) {
    assert!(pixels.len() % 4 == 0);
    let mut chunks = pixels.chunks_exact_mut(32);

    // This should vectorize faster than a naive mem swap
    for chunk in &mut chunks {
        let tmp = swizzle.map(|i| chunk[i]);
        chunk.copy_from_slice(&tmp[..])
    }

    let remainder = chunks.into_remainder();
    for chunk in remainder.chunks_exact_mut(4) {
        let argb = [
            chunk[swizzle[0]],
            chunk[swizzle[1]],
            chunk[swizzle[2]],
            chunk[swizzle[3]],
        ];
        chunk.copy_from_slice(&argb[..])
    }
}

const fn generate_swizzle<const LEN: usize>(swizzle: [usize; 4]) -> [usize; LEN] {
    assert!(LEN % 4 == 0, "length of swizzle must be divisible by 4");
    let mut out: [usize; LEN] = [0; LEN];

    let mut index = 0;
    while index < LEN {
        let chunk = [index, index + 1, index + 2, index + 3];
        out[index + 0] = chunk[swizzle[0]];
        out[index + 1] = chunk[swizzle[1]];
        out[index + 2] = chunk[swizzle[2]];
        out[index + 3] = chunk[swizzle[3]];

        index += 4;
    }

    out
}

#[cfg(test)]
mod test {
    use crate::image::generate_swizzle;

    #[test]
    pub fn generate_normal_swizzle() {
        let swizzle = generate_swizzle::<32>([0, 1, 2, 3]);
        assert_eq!(
            swizzle,
            #[rustfmt::skip]
            [
                0, 1, 2, 3,
                4, 5, 6, 7,
                8, 9, 10, 11,
                12, 13, 14, 15,
                16, 17, 18, 19,
                20, 21, 22, 23,
                24, 25, 26, 27,
                28, 29, 30, 31
            ]
        )
    }

    #[test]
    pub fn generate_argb_swizzle() {
        let swizzle = generate_swizzle::<32>([3, 0, 1, 2]);
        assert_eq!(
            swizzle,
            #[rustfmt::skip]
            [
                3, 0, 1, 2,
                7, 4, 5, 6,
                11, 8, 9, 10,
                15, 12, 13, 14,
                19, 16, 17, 18,
                23, 20, 21, 22,
                27, 24, 25, 26,
                31, 28, 29, 30
            ]
        )
    }
}
