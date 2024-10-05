use crate::quad::{DEFAULT_MVP, IDENTITY_MVP};
use librashader_common::{GetSize, Size, Viewport};
use num_traits::{zero, AsPrimitive, Num};
use std::borrow::Borrow;

/// An internal render target to which pass colour attachments are made.
#[derive(Debug, Clone)]
pub struct RenderTarget<'a, T, C = f32>
where
    C: Num,
{
    /// The x-coordinate of the viewport to render to.
    pub x: C,
    /// The y-coordinate of the viewport to render to.
    pub y: C,
    /// The MVP to pass to the shader for this render pass.
    pub mvp: &'a [f32; 16],
    /// The output surface for the pass.
    pub output: &'a T,
    /// The extent of the render target, starting from the origin defined
    /// by x and y.
    pub size: Size<u32>,
}

impl<'a, T: GetSize<u32>, C: Num> RenderTarget<'a, T, C> {
    /// Create a new render target.
    pub fn new(output: &'a T, mvp: &'a [f32; 16], x: C, y: C) -> Result<Self, T::Error> {
        Ok(RenderTarget {
            output,
            mvp,
            x,
            y,
            size: output.size()?,
        })
    }

    /// Create a render target with the identity MVP.
    pub fn identity(output: &'a T) -> Result<Self, T::Error> {
        Self::offscreen(output, IDENTITY_MVP)
    }

    /// Create an offscreen render target with the given MVP.
    pub fn offscreen(output: &'a T, mvp: &'a [f32; 16]) -> Result<Self, T::Error> {
        Self::new(output, mvp, zero(), zero())
    }
}

impl<'a, T, C: Num + Copy + 'static> RenderTarget<'a, T, C>
where
    f32: AsPrimitive<C>,
{
    /// Create a viewport render target.
    pub fn viewport(viewport: &'a Viewport<'a, impl Borrow<T>>) -> Self {
        RenderTarget {
            output: viewport.output.borrow(),
            mvp: viewport.mvp.unwrap_or(DEFAULT_MVP),
            x: viewport.x.as_(),
            y: viewport.y.as_(),
            size: viewport.size,
        }
    }

    /// Create a viewport render target with the given output.
    pub fn viewport_with_output<S>(output: &'a T, viewport: &'a Viewport<'a, S>) -> Self {
        RenderTarget {
            output,
            mvp: viewport.mvp.unwrap_or(DEFAULT_MVP),
            x: viewport.x.as_(),
            y: viewport.y.as_(),
            size: viewport.size,
        }
    }
}
