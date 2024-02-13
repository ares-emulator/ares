/// Different type of quad to render to depending on pass type
pub enum QuadType {
    /// Offscreen, intermediate passes.
    Offscreen,
    /// Final pass to render target.
    Final,
}

/// Identity MVP for use in intermediate passes.
#[rustfmt::skip]
pub static IDENTITY_MVP: &[f32; 16] = &[
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
];

/// Default MVP for use when rendering to the render target.
#[rustfmt::skip]
pub static DEFAULT_MVP: &[f32; 16] = &[
    2f32, 0.0, 0.0, 0.0,
    0.0, 2.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    -1.0, -1.0, 0.0, 1.0,
];
