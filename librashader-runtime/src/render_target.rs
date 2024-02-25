use crate::quad::{DEFAULT_MVP, IDENTITY_MVP};
use librashader_common::Viewport;
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
}

impl<'a, T, C: Num> RenderTarget<'a, T, C> {
    /// Create a new render target.
    pub fn new(output: &'a T, mvp: &'a [f32; 16], x: C, y: C) -> Self {
        RenderTarget { output, mvp, x, y }
    }

    /// Create a render target with the identity MVP.
    pub fn identity(output: &'a T) -> Self {
        Self::offscreen(output, IDENTITY_MVP)
    }

    /// Create an offscreen render target with the given MVP.
    pub fn offscreen(output: &'a T, mvp: &'a [f32; 16]) -> Self {
        Self::new(output, mvp, zero(), zero())
    }
}

impl<'a, T, C: Num + Copy + 'static> RenderTarget<'a, T, C>
where
    f32: AsPrimitive<C>,
{
    /// Create a viewport render target.
    pub fn viewport(viewport: &'a Viewport<'a, impl Borrow<T>>) -> Self {
        Self::new(
            viewport.output.borrow(),
            viewport.mvp.unwrap_or(DEFAULT_MVP),
            viewport.x.as_(),
            viewport.y.as_(),
        )
    }

    /// Create a viewport render target with the given output.
    pub fn viewport_with_output<S>(output: &'a T, viewport: &'a Viewport<'a, S>) -> Self {
        Self::new(
            output,
            viewport.mvp.unwrap_or(DEFAULT_MVP),
            viewport.x.as_(),
            viewport.y.as_(),
        )
    }
}
