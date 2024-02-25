use crate::uniforms::{BindUniform, NoUniformBinder, UniformStorage};
use librashader_common::map::FastHashMap;
use librashader_common::Size;
use librashader_preprocess::ShaderParameter;
use librashader_reflect::reflect::semantics::{
    BindingMeta, MemberOffset, Semantic, TextureBinding, TextureSemantics, UniformBinding,
    UniformMeta, UniqueSemantics,
};
use std::ops::{Deref, DerefMut};

/// Trait for input textures used during uniform binding,
pub trait TextureInput {
    /// Gets the size of this input texture.
    fn size(&self) -> Size<u32>;
}

/// A uniform member offset with context that needs to be resolved.
pub trait ContextOffset<H, C>
where
    H: BindUniform<C, f32>,
    H: BindUniform<C, u32>,
    H: BindUniform<C, i32>,
    H: for<'a> BindUniform<C, &'a [f32; 4]>,
    H: for<'a> BindUniform<C, &'a [f32; 16]>,
{
    /// Gets the `MemberOffset` part of the offset.
    fn offset(&self) -> MemberOffset;

    /// Gets the context part of the offset.
    fn context(&self) -> C;
}

impl<H> ContextOffset<H, Option<()>> for MemberOffset
where
    H: BindUniform<Option<()>, f32>,
    H: BindUniform<Option<()>, u32>,
    H: BindUniform<Option<()>, i32>,
    H: for<'a> BindUniform<Option<()>, &'a [f32; 4]>,
    H: for<'a> BindUniform<Option<()>, &'a [f32; 16]>,
{
    fn offset(&self) -> MemberOffset {
        *self
    }

    fn context(&self) -> Option<()> {
        None
    }
}

/// Inputs to binding semantics
pub struct UniformInputs<'a> {
    /// MVP
    pub mvp: &'a [f32; 16],
    /// FrameCount
    pub frame_count: u32,
    /// Rotation
    pub rotation: u32,
    /// TotalSubFrames
    pub total_subframes: u32,
    /// CurrentSubFrame
    pub current_subframe: u32,
    /// FrameDirection
    pub frame_direction: i32,
    /// OutputSize
    pub framebuffer_size: Size<u32>,
    /// FinalViewportSize
    pub viewport_size: Size<u32>,
}

/// Trait that abstracts binding of semantics to shader uniforms.
pub trait BindSemantics<H = NoUniformBinder, C = Option<()>, U = Box<[u8]>, P = Box<[u8]>>
where
    C: Copy,
    U: Deref<Target = [u8]> + DerefMut,
    P: Deref<Target = [u8]> + DerefMut,
    H: BindUniform<C, f32>,
    H: BindUniform<C, u32>,
    H: BindUniform<C, i32>,
    H: for<'b> BindUniform<C, &'b [f32; 4]>,
    H: for<'b> BindUniform<C, &'b [f32; 16]>,
{
    /// The type of the input texture used for semantic binding.
    type InputTexture: TextureInput;

    /// The set of texture samplers available.
    type SamplerSet;

    /// The descriptor set or object that holds sampler and texture bindings.
    type DescriptorSet<'a>;

    /// The device context containing the state of the graphics processor.
    type DeviceContext;

    /// The type of uniform offsets to use.
    type UniformOffset: ContextOffset<H, C>;

    /// Bind a texture to the input descriptor set
    fn bind_texture<'a>(
        descriptors: &mut Self::DescriptorSet<'a>,
        samplers: &Self::SamplerSet,
        binding: &TextureBinding,
        texture: &Self::InputTexture,
        device: &Self::DeviceContext,
    );

    #[allow(clippy::too_many_arguments)]
    /// Write uniform and texture semantics to the provided storages.
    fn bind_semantics<'a>(
        device: &Self::DeviceContext,
        sampler_set: &Self::SamplerSet,
        uniform_storage: &mut UniformStorage<H, C, U, P>,
        descriptor_set: &mut Self::DescriptorSet<'a>,
        uniform_inputs: UniformInputs<'_>,
        original: &Self::InputTexture,
        source: &Self::InputTexture,
        uniform_bindings: &FastHashMap<UniformBinding, Self::UniformOffset>,
        texture_meta: &FastHashMap<Semantic<TextureSemantics>, TextureBinding>,
        pass_outputs: impl Iterator<Item = Option<impl AsRef<Self::InputTexture>>>,
        pass_feedback: impl Iterator<Item = Option<impl AsRef<Self::InputTexture>>>,
        original_history: impl Iterator<Item = Option<impl AsRef<Self::InputTexture>>>,
        lookup_textures: impl Iterator<Item = (usize, impl AsRef<Self::InputTexture>)>,
        parameter_defaults: &FastHashMap<String, ShaderParameter>,
        runtime_parameters: &FastHashMap<String, f32>,
    ) {
        // Bind MVP
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::MVP.into()) {
            uniform_storage.bind_mat4(offset.offset(), uniform_inputs.mvp, offset.context());
        }

        // Bind OutputSize
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::Output.into()) {
            uniform_storage.bind_vec4(
                offset.offset(),
                uniform_inputs.framebuffer_size,
                offset.context(),
            );
        }

        // bind FinalViewportSize
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::FinalViewport.into()) {
            uniform_storage.bind_vec4(
                offset.offset(),
                uniform_inputs.viewport_size,
                offset.context(),
            );
        }

        // bind FrameCount
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::FrameCount.into()) {
            uniform_storage.bind_scalar(
                offset.offset(),
                uniform_inputs.frame_count,
                offset.context(),
            );
        }

        // bind FrameDirection
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::FrameDirection.into()) {
            uniform_storage.bind_scalar(
                offset.offset(),
                uniform_inputs.frame_direction,
                offset.context(),
            );
        }

        // bind Rotation
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::Rotation.into()) {
            uniform_storage.bind_scalar(offset.offset(), uniform_inputs.rotation, offset.context());
        }

        // bind TotalSubFrames
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::TotalSubFrames.into()) {
            uniform_storage.bind_scalar(
                offset.offset(),
                uniform_inputs.total_subframes,
                offset.context(),
            );
        }

        // bind CurrentSubFrames
        if let Some(offset) = uniform_bindings.get(&UniqueSemantics::CurrentSubFrame.into()) {
            uniform_storage.bind_scalar(
                offset.offset(),
                uniform_inputs.current_subframe,
                offset.context(),
            );
        }

        // bind Original sampler
        if let Some(binding) = texture_meta.get(&TextureSemantics::Original.semantics(0)) {
            Self::bind_texture(descriptor_set, sampler_set, binding, original, device);
        }

        // bind OriginalSize
        if let Some(offset) = uniform_bindings.get(&TextureSemantics::Original.semantics(0).into())
        {
            uniform_storage.bind_vec4(offset.offset(), original.size(), offset.context());
        }

        // bind Source sampler
        if let Some(binding) = texture_meta.get(&TextureSemantics::Source.semantics(0)) {
            Self::bind_texture(descriptor_set, sampler_set, binding, source, device);
        }

        // bind SourceSize
        if let Some(offset) = uniform_bindings.get(&TextureSemantics::Source.semantics(0).into()) {
            uniform_storage.bind_vec4(offset.offset(), source.size(), offset.context());
        }

        // OriginalHistory0 aliases OriginalHistory

        // bind OriginalHistory0 sampler
        if let Some(binding) = texture_meta.get(&TextureSemantics::OriginalHistory.semantics(0)) {
            Self::bind_texture(descriptor_set, sampler_set, binding, original, device);
        }

        // bind OriginalHistory0Size
        if let Some(offset) =
            uniform_bindings.get(&TextureSemantics::OriginalHistory.semantics(0).into())
        {
            uniform_storage.bind_vec4(offset.offset(), original.size(), offset.context());
        }

        // bind OriginalHistory1-..
        for (index, history) in original_history.enumerate() {
            let Some(history) = history else {
                continue;
            };

            let history = history.as_ref();

            if let Some(binding) =
                texture_meta.get(&TextureSemantics::OriginalHistory.semantics(index + 1))
            {
                Self::bind_texture(descriptor_set, sampler_set, binding, history, device);
            }

            if let Some(offset) = uniform_bindings.get(
                &TextureSemantics::OriginalHistory
                    .semantics(index + 1)
                    .into(),
            ) {
                uniform_storage.bind_vec4(offset.offset(), history.size(), offset.context());
            }
        }

        // bind PassOutput0..
        // The caller should be responsible for limiting this up to
        // pass_index
        for (index, output) in pass_outputs.enumerate() {
            let Some(output) = output else {
                continue;
            };

            let output = output.as_ref();

            if let Some(binding) = texture_meta.get(&TextureSemantics::PassOutput.semantics(index))
            {
                Self::bind_texture(descriptor_set, sampler_set, binding, output, device);
            }

            if let Some(offset) =
                uniform_bindings.get(&TextureSemantics::PassOutput.semantics(index).into())
            {
                uniform_storage.bind_vec4(offset.offset(), output.size(), offset.context());
            }
        }

        // bind PassFeedback0..
        for (index, feedback) in pass_feedback.enumerate() {
            let Some(output) = feedback else {
                continue;
            };

            let feedback = output.as_ref();

            if let Some(binding) =
                texture_meta.get(&TextureSemantics::PassFeedback.semantics(index))
            {
                Self::bind_texture(descriptor_set, sampler_set, binding, feedback, device);
            }

            if let Some(offset) =
                uniform_bindings.get(&TextureSemantics::PassFeedback.semantics(index).into())
            {
                uniform_storage.bind_vec4(offset.offset(), feedback.size(), offset.context());
            }
        }

        // bind User parameters
        for (id, offset) in uniform_bindings
            .iter()
            .filter_map(|(binding, value)| match binding {
                UniformBinding::Parameter(id) => Some((id, value)),
                _ => None,
            })
        {
            let id = id.as_str();

            let default = parameter_defaults.get(id).map_or(0f32, |f| f.initial);

            let value = *runtime_parameters.get(id).unwrap_or(&default);

            uniform_storage.bind_scalar(offset.offset(), value, offset.context());
        }

        // bind luts
        for (index, lut) in lookup_textures {
            let lut = lut.as_ref();
            if let Some(binding) = texture_meta.get(&TextureSemantics::User.semantics(index)) {
                Self::bind_texture(descriptor_set, sampler_set, binding, lut, device);
            }

            if let Some(offset) =
                uniform_bindings.get(&TextureSemantics::User.semantics(index).into())
            {
                uniform_storage.bind_vec4(offset.offset(), lut.size(), offset.context());
            }
        }
    }
}

/// Trait for objects that can be used to create a binding map.
pub trait BindingUtil {
    /// Create the uniform binding map with the given reflection information.
    fn create_binding_map<T>(
        &self,
        f: impl Fn(&dyn UniformMeta) -> T,
    ) -> FastHashMap<UniformBinding, T>;

    /// Calculate the number of required images for history.
    fn calculate_required_history<'a>(pass_meta: impl Iterator<Item = &'a Self>) -> usize
    where
        Self: 'a;
}

impl BindingUtil for BindingMeta {
    fn create_binding_map<T>(
        &self,
        f: impl Fn(&dyn UniformMeta) -> T,
    ) -> FastHashMap<UniformBinding, T> {
        let mut uniform_bindings = FastHashMap::default();
        for param in self.parameter_meta.values() {
            uniform_bindings.insert(UniformBinding::Parameter(param.id.clone()), f(param));
        }

        for (semantics, param) in &self.unique_meta {
            uniform_bindings.insert(UniformBinding::SemanticVariable(*semantics), f(param));
        }

        for (semantics, param) in &self.texture_size_meta {
            uniform_bindings.insert(UniformBinding::TextureSize(*semantics), f(param));
        }

        uniform_bindings
    }

    fn calculate_required_history<'a>(pass_meta: impl Iterator<Item = &'a Self>) -> usize
    where
        Self: 'a,
    {
        let mut required_images = 0;

        for pass in pass_meta {
            // If a shader uses history size, but not history, we still need to keep the texture.
            let texture_max_index = pass
                .texture_meta
                .iter()
                .filter(|(semantics, _)| semantics.semantics == TextureSemantics::OriginalHistory)
                .map(|(semantic, _)| semantic.index)
                .fold(0, std::cmp::max);
            let texture_size_max_index = pass
                .texture_size_meta
                .iter()
                .filter(|(semantics, _)| semantics.semantics == TextureSemantics::OriginalHistory)
                .map(|(semantic, _)| semantic.index)
                .fold(0, std::cmp::max);

            required_images = std::cmp::max(required_images, texture_max_index);
            required_images = std::cmp::max(required_images, texture_size_max_index);
        }

        required_images
    }
}

#[macro_export]
macro_rules! impl_default_frame_options {
    ($ty:ident) => {
        /// Options for each frame.
        #[repr(C)]
        #[derive(Debug, Clone)]
        pub struct $ty {
            /// Whether or not to clear the history buffers.
            pub clear_history: bool,
            /// The direction of rendering.
            /// -1 indicates that the frames are played in reverse order.
            pub frame_direction: i32,
            /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 4 = 270deg.
            pub rotation: u32,
            /// The total number of subframes ran. Default is 1.
            pub total_subframes: u32,
            // The current sub frame. Default is 1.
            pub current_subframe: u32,
        }

        impl Default for $ty {
            fn default() -> Self {
                Self {
                    clear_history: false,
                    frame_direction: 1,
                    rotation: 0,
                    total_subframes: 1,
                    current_subframe: 1,
                }
            }
        }
    };
}
