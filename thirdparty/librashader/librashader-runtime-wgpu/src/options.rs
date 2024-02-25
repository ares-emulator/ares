//! wgpu shader runtime options.

use librashader_runtime::impl_default_frame_options;
impl_default_frame_options!(FrameOptionsWgpu);

/// Options for filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct FilterChainOptionsWgpu {
    /// Whether or not to explicitly disable mipmap generation regardless of shader preset settings.
    pub force_no_mipmaps: bool,
}
