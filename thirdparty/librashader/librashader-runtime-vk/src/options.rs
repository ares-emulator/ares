//! Vulkan shader runtime options.

use librashader_runtime::impl_default_frame_options;
impl_default_frame_options!(FrameOptionsVulkan);

/// Options for filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct FilterChainOptionsVulkan {
    /// The number of frames in flight to keep. If zero, defaults to three.
    pub frames_in_flight: u32,
    /// Whether or not to explicitly disable mipmap generation regardless of shader preset settings.
    pub force_no_mipmaps: bool,
    /// Use dynamic rendering instead of explicit render pass objects.
    /// It is recommended if possible to use dynamic rendering,
    /// because render-pass mode will create new framebuffers per pass.
    pub use_dynamic_rendering: bool,
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    pub disable_cache: bool,
}
