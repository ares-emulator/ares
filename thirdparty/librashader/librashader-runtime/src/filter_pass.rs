use librashader_common::ImageFormat;
use librashader_presets::ShaderPassConfig;

/// Trait for metadata about a filter pass.
pub trait FilterPassMeta {
    /// Gets the format of the framebuffer for the pass.
    fn framebuffer_format(&self) -> ImageFormat;

    /// Gets a reference to the filter pass config.
    fn config(&self) -> &ShaderPassConfig;

    /// Gets the format of the filter pass framebuffer.
    #[inline(always)]
    fn get_format(&self) -> ImageFormat {
        let fb_format = self.framebuffer_format();
        if let Some(format) = self.config().get_format_override() {
            format
        } else if fb_format == ImageFormat::Unknown {
            ImageFormat::R8G8B8A8Unorm
        } else {
            fb_format
        }
    }
}
