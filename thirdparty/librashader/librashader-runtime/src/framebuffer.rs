use crate::binding::BindingUtil;
use librashader_reflect::reflect::semantics::BindingMeta;
use std::collections::VecDeque;

/// Helper to initialize framebuffers in a graphics API agnostic way.
pub struct FramebufferInit<'a, F, I, E> {
    owned_generator: &'a dyn Fn() -> Result<F, E>,
    input_generator: &'a dyn Fn() -> I,
    required_history: usize,
    filters_count: usize,
}

impl<'a, F, I, E> FramebufferInit<'a, F, I, E> {
    /// Create a new framebuffer initializer with the given
    /// closures to create owned framebuffers and image views.
    pub fn new(
        filters: impl Iterator<Item = &'a BindingMeta> + ExactSizeIterator,
        owned_generator: &'a dyn Fn() -> Result<F, E>,
        input_generator: &'a dyn Fn() -> I,
    ) -> Self {
        let filters_count = filters.len();
        let required_history = BindingMeta::calculate_required_history(filters);
        Self {
            owned_generator,
            input_generator,
            filters_count,
            required_history,
        }
    }

    /// Initialize history framebuffers and views.
    pub fn init_history(&self) -> Result<(VecDeque<F>, Box<[I]>), E> {
        init_history(
            self.required_history,
            self.owned_generator,
            self.input_generator,
        )
    }

    /// Initialize output framebuffers and views.
    pub fn init_output_framebuffers(&self) -> Result<(Box<[F]>, Box<[I]>), E> {
        init_output_framebuffers(
            self.filters_count,
            self.owned_generator,
            self.input_generator,
        )
    }
}

fn init_history<'a, F, I, E>(
    required_images: usize,
    owned_generator: impl Fn() -> Result<F, E>,
    input_generator: impl Fn() -> I,
) -> Result<(VecDeque<F>, Box<[I]>), E> {
    if required_images <= 1 {
        return Ok((VecDeque::new(), Box::new([])));
    }

    let mut framebuffers = VecDeque::with_capacity(required_images);
    framebuffers.resize_with(required_images, owned_generator);

    let framebuffers = framebuffers
        .into_iter()
        .collect::<Result<VecDeque<F>, E>>()?;

    let mut history_textures = Vec::new();
    history_textures.resize_with(required_images, input_generator);

    Ok((framebuffers, history_textures.into_boxed_slice()))
}

fn init_output_framebuffers<F, I, E>(
    len: usize,
    owned_generator: impl Fn() -> Result<F, E>,
    input_generator: impl Fn() -> I,
) -> Result<(Box<[F]>, Box<[I]>), E> {
    let mut output_framebuffers = Vec::new();
    output_framebuffers.resize_with(len, owned_generator);

    // resolve all results
    let output_framebuffers = output_framebuffers
        .into_iter()
        .collect::<Result<Vec<F>, E>>()?;

    let mut output_textures = Vec::new();
    output_textures.resize_with(len, input_generator);

    Ok((
        output_framebuffers.into_boxed_slice(),
        output_textures.into_boxed_slice(),
    ))
}
