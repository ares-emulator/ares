use crate::error::{FilterChainError, Result};
use crate::filter_chain::chain::FilterChainImpl;
use crate::filter_chain::inner::FilterChainDispatch;
use crate::options::{FilterChainOptionsGL, FrameOptionsGL};
use crate::GLImage;
use librashader_presets::ShaderPreset;
use std::panic::catch_unwind;
use std::path::Path;
use std::sync::Arc;

mod chain;
mod inner;
mod parameters;

pub(crate) use chain::FilterCommon;
use librashader_common::Viewport;
use librashader_pack::ShaderPresetPack;
use librashader_presets::context::VideoDriver;

/// An OpenGL filter chain.
pub struct FilterChainGL {
    pub(in crate::filter_chain) filter: FilterChainDispatch,
}

impl FilterChainGL {
    /// Load a filter chain from a pre-parsed and loaded `ShaderPresetPack`.
    pub unsafe fn load_from_preset(
        preset: ShaderPreset,
        ctx: Arc<glow::Context>,
        options: Option<&FilterChainOptionsGL>,
    ) -> Result<Self> {
        let preset = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        unsafe { Self::load_from_pack(preset, ctx, options) }
    }

    /// Load a filter chain from a pre-parsed and loaded `ShaderPresetPack`.
    pub unsafe fn load_from_pack(
        preset: ShaderPresetPack,
        ctx: Arc<glow::Context>,
        options: Option<&FilterChainOptionsGL>,
    ) -> Result<Self> {
        let result = catch_unwind(|| {
            if options.is_some_and(|options| options.use_dsa) {
                return Ok(Self {
                    filter: FilterChainDispatch::DirectStateAccess(unsafe {
                        FilterChainImpl::load_from_pack(preset, ctx, options)?
                    }),
                });
            }
            Ok(Self {
                filter: FilterChainDispatch::Compatibility(unsafe {
                    FilterChainImpl::load_from_pack(preset, ctx, options)?
                }),
            })
        });
        result.unwrap_or_else(|_| Err(FilterChainError::GLLoadError))
    }

    /// Load the shader preset at the given path into a filter chain.
    pub unsafe fn load_from_path(
        path: impl AsRef<Path>,
        ctx: Arc<glow::Context>,
        options: Option<&FilterChainOptionsGL>,
    ) -> Result<Self> {
        // load passes from preset
        let preset = ShaderPreset::try_parse_with_driver_context(path, VideoDriver::GlCore)?;
        unsafe { Self::load_from_preset(preset, ctx, options) }
    }

    /// Process a frame with the input image.
    ///
    /// When this frame returns, `GL_FRAMEBUFFER` is bound to 0 if not using Direct State Access.
    /// Otherwise, it is untouched.
    pub unsafe fn frame(
        &mut self,
        input: &GLImage,
        viewport: &Viewport<&GLImage>,
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

    /// Get the GL context associated with this filter chain
    pub fn get_context(&self) -> &Arc<glow::Context> {
        match &self.filter {
            FilterChainDispatch::DirectStateAccess(p) => &p.common.context,
            FilterChainDispatch::Compatibility(p) => &p.common.context,
        }
    }
}
