//! Direct3D 12 shader runtime options.

use librashader_runtime::impl_default_frame_options;
impl_default_frame_options!(FrameOptionsD3D12);

/// Options for Direct3D 12 filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct FilterChainOptionsD3D12 {
    /// Force the HLSL shader pipeline. This may reduce shader compatibility.
    pub force_hlsl_pipeline: bool,

    /// Whether or not to explicitly disable mipmap
    /// generation for intermediate passes regardless
    /// of shader preset settings.
    pub force_no_mipmaps: bool,

    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    pub disable_cache: bool,
}
