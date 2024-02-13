/// The rendering output of a filter chain.
pub struct Viewport<'a, T> {
    /// The x offset to start rendering from.
    pub x: f32,
    /// The y offset to begin rendering from.
    pub y: f32,
    /// An optional pointer to an MVP to use when rendering
    /// to the viewport.
    pub mvp: Option<&'a [f32; 16]>,
    /// The output handle to render the final image to.
    pub output: T,
}
