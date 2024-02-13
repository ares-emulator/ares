use std::panic::catch_unwind;
use std::path::Path;

use crate::error::{FilterChainError, Result};
use crate::filter_chain::filter_impl::FilterChainImpl;
use crate::filter_chain::inner::FilterChainDispatch;
use crate::options::{FilterChainOptionsGL, FrameOptionsGL};
use crate::{GLFramebuffer, GLImage};
use librashader_presets::ShaderPreset;

mod filter_impl;
mod inner;
mod parameters;

pub(crate) use filter_impl::FilterCommon;
use librashader_common::Viewport;
use librashader_presets::context::VideoDriver;

/// An OpenGL filter chain.
pub struct FilterChainGL {
    pub(in crate::filter_chain) filter: FilterChainDispatch,
}

impl FilterChainGL {
    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub unsafe fn load_from_preset(
        preset: ShaderPreset,
        options: Option<&FilterChainOptionsGL>,
    ) -> Result<Self> {
        let result = catch_unwind(|| {
            if let Some(options) = options
                && options.use_dsa
            {
                return Ok(Self {
                    filter: FilterChainDispatch::DirectStateAccess(unsafe {
                        FilterChainImpl::load_from_preset(preset, Some(options))?
                    }),
                });
            }
            Ok(Self {
                filter: FilterChainDispatch::Compatibility(unsafe {
                    FilterChainImpl::load_from_preset(preset, options)?
                }),
            })
        });
        result.unwrap_or_else(|_| Err(FilterChainError::GLLoadError))
    }

    /// Load the shader preset at the given path into a filter chain.
    pub unsafe fn load_from_path(
        path: impl AsRef<Path>,
        options: Option<&FilterChainOptionsGL>,
    ) -> Result<Self> {
        // load passes from preset
        let preset = ShaderPreset::try_parse_with_driver_context(path, VideoDriver::GlCore)?;
        unsafe { Self::load_from_preset(preset, options) }
    }

    /// Process a frame with the input image.
    ///
    /// When this frame returns, `GL_FRAMEBUFFER` is bound to 0 if not using Direct State Access.
    /// Otherwise, it is untouched.
    pub unsafe fn frame(
        &mut self,
        input: &GLImage,
        viewport: &Viewport<&GLFramebuffer>,
        frame_count: usize,
        options: Option<&FrameOptionsGL>,
    ) -> Result<()> {
        match &mut self.filter {
            FilterChainDispatch::DirectStateAccess(p) => unsafe {
                p.frame(frame_count, viewport, input, options)
            },
            FilterChainDispatch::Compatibility(p) => unsafe {
                p.frame(frame_count, viewport, input, options)
            },
        }
    }
}
