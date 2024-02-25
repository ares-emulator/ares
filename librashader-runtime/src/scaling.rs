use crate::filter_pass::FilterPassMeta;
use crate::scaling;
use librashader_common::{ImageFormat, Size};
use librashader_presets::{Scale2D, ScaleFactor, ScaleType, Scaling};
use num_traits::AsPrimitive;
use std::ops::Mul;

/// Trait for size scaling relative to the viewport.
pub trait ViewportSize<T>
where
    T: Mul<ScaleFactor, Output = f32> + Copy + 'static,
    f32: AsPrimitive<T>,
{
    /// Produce a `Size<T>` scaled with the input scaling options.
    fn scale_viewport(self, scaling: Scale2D, viewport: Size<T>, original: Size<T>) -> Size<T>;
}

impl<T> ViewportSize<T> for Size<T>
where
    T: Mul<ScaleFactor, Output = f32> + Copy + 'static,
    f32: AsPrimitive<T>,
{
    fn scale_viewport(self, scaling: Scale2D, viewport: Size<T>, original: Size<T>) -> Size<T>
    where
        T: Mul<ScaleFactor, Output = f32> + Copy + 'static,
        f32: AsPrimitive<T>,
    {
        scaling::scale(scaling, self, viewport, original)
    }
}

/// Trait for size scaling relating to mipmap generation.
pub trait MipmapSize<T> {
    /// Calculate the number of mipmap levels for a given size.
    fn calculate_miplevels(self) -> T;

    /// Scale the size according to the given mipmap level.
    fn scale_mipmap(self, miplevel: T) -> Size<T>;
}

impl MipmapSize<u32> for Size<u32> {
    fn calculate_miplevels(self) -> u32 {
        let mut size = std::cmp::max(self.width, self.height);
        let mut levels = 0;
        while size != 0 {
            levels += 1;
            size >>= 1;
        }

        levels
    }

    fn scale_mipmap(self, miplevel: u32) -> Size<u32> {
        let scaled_width = std::cmp::max(self.width >> miplevel, 1);
        let scaled_height = std::cmp::max(self.height >> miplevel, 1);
        Size::new(scaled_width, scaled_height)
    }
}

fn scale<T>(scaling: Scale2D, source: Size<T>, viewport: Size<T>, original: Size<T>) -> Size<T>
where
    T: Mul<ScaleFactor, Output = f32> + Copy + 'static,
    f32: AsPrimitive<T>,
{
    let width = match scaling.x {
        Scaling {
            scale_type: ScaleType::Input,
            factor,
        } => source.width * factor,
        Scaling {
            scale_type: ScaleType::Absolute,
            factor,
        } => factor.into(),
        Scaling {
            scale_type: ScaleType::Viewport,
            factor,
        } => viewport.width * factor,
        Scaling {
            scale_type: ScaleType::Original,
            factor,
        } => original.width * factor,
    };

    let height = match scaling.y {
        Scaling {
            scale_type: ScaleType::Input,
            factor,
        } => source.height * factor,
        Scaling {
            scale_type: ScaleType::Absolute,
            factor,
        } => factor.into(),
        Scaling {
            scale_type: ScaleType::Viewport,
            factor,
        } => viewport.height * factor,
        Scaling {
            scale_type: ScaleType::Original,
            factor,
        } => original.height * factor,
    };

    Size {
        width: width.round().as_(),
        height: height.round().as_(),
    }
}

/// Trait for owned framebuffer objects that can be scaled.
pub trait ScaleFramebuffer<T = ()> {
    type Error;
    type Context;
    /// Scale the framebuffer according to the provided parameters, returning the new size.
    fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        should_mipmap: bool,
        context: &Self::Context,
    ) -> Result<Size<u32>, Self::Error>;

    /// Scale framebuffers with default context.
    #[inline(always)]
    fn scale_framebuffers<P>(
        source_size: Size<u32>,
        viewport_size: Size<u32>,
        original_size: Size<u32>,
        output: &mut [Self],
        feedback: &mut [Self],
        passes: &[P],
        callback: Option<&mut dyn FnMut(usize, &P, &Self, &Self) -> Result<(), Self::Error>>,
    ) -> Result<(), Self::Error>
    where
        Self: Sized,
        Self::Context: Default,
        P: FilterPassMeta,
    {
        scale_framebuffers_with_context_callback::<T, Self, Self::Error, Self::Context, _>(
            source_size,
            viewport_size,
            original_size,
            output,
            feedback,
            passes,
            &Self::Context::default(),
            callback,
        )
    }

    /// Scale framebuffers with user provided context.
    #[inline(always)]
    fn scale_framebuffers_with_context<P>(
        source_size: Size<u32>,
        viewport_size: Size<u32>,
        original_size: Size<u32>,
        output: &mut [Self],
        feedback: &mut [Self],
        passes: &[P],
        context: &Self::Context,
        callback: Option<&mut dyn FnMut(usize, &P, &Self, &Self) -> Result<(), Self::Error>>,
    ) -> Result<(), Self::Error>
    where
        Self: Sized,
        P: FilterPassMeta,
    {
        scale_framebuffers_with_context_callback::<T, Self, Self::Error, Self::Context, _>(
            source_size,
            viewport_size,
            original_size,
            output,
            feedback,
            passes,
            context,
            callback,
        )
    }
}

/// Scale framebuffers according to the pass configs, source and viewport size
/// passing a context into the scale function and a callback for each framebuffer rescale.
#[inline(always)]
fn scale_framebuffers_with_context_callback<T, F, E, C, P>(
    source_size: Size<u32>,
    viewport_size: Size<u32>,
    original_size: Size<u32>,
    output: &mut [F],
    feedback: &mut [F],
    passes: &[P],
    context: &C,
    mut callback: Option<&mut dyn FnMut(usize, &P, &F, &F) -> Result<(), E>>,
) -> Result<(), E>
where
    F: ScaleFramebuffer<T, Context = C, Error = E>,
    P: FilterPassMeta,
{
    assert_eq!(output.len(), feedback.len());
    let mut iterator = passes.iter().enumerate().peekable();
    let mut target_size = source_size;
    while let Some((index, pass)) = iterator.next() {
        let should_mipmap = iterator
            .peek()
            .map_or(false, |(_, p)| p.config().mipmap_input);

        let next_size = output[index].scale(
            pass.config().scaling.clone(),
            pass.get_format(),
            &viewport_size,
            &target_size,
            &original_size,
            should_mipmap,
            context,
        )?;

        feedback[index].scale(
            pass.config().scaling.clone(),
            pass.get_format(),
            &viewport_size,
            &target_size,
            &original_size,
            should_mipmap,
            context,
        )?;

        target_size = next_size;

        if let Some(callback) = callback.as_mut() {
            callback(index, pass, &output[index], &feedback[index])?;
        }
    }

    Ok(())
}
