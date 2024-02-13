//! Direct3D 11 shader runtime options.

use librashader_runtime::impl_default_frame_options;
impl_default_frame_options!(FrameOptionsD3D11);

/// Options for Direct3D 11 filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct FilterChainOptionsD3D11 {
    /// Whether or not to explicitly disable mipmap
    /// generation regardless of shader preset settings.
    pub force_no_mipmaps: bool,
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    pub disable_cache: bool,
}
