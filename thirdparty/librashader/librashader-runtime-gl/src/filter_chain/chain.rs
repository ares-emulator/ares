use crate::binding::{GlUniformStorage, UniformLocation, VariableLocation};
use crate::error::FilterChainError;
use crate::filter_pass::{FilterPass, UniformOffset};
use crate::gl::{
    CompileProgram, DrawQuad, FramebufferInterface, GLFramebuffer, GLInterface, LoadLut,
    OutputFramebuffer, UboRing,
};
use crate::options::{FilterChainOptionsGL, FrameOptionsGL};
use crate::samplers::SamplerSet;
use crate::texture::InputTexture;
use crate::util::{gl_get_version, gl_u16_to_version};
use crate::{error, GLImage};
use librashader_common::Viewport;

use librashader_reflect::back::glsl::GlslVersion;
use librashader_reflect::back::targets::GLSL;
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::SpirvCompilation;
use librashader_reflect::reflect::semantics::{ShaderSemantics, UniformMeta};

use glow::HasContext;
use librashader_cache::CachedCompilation;
use librashader_common::map::FastHashMap;
use librashader_pack::{PassResource, ShaderPresetPack, TextureResource};
use librashader_reflect::reflect::cross::SpirvCross;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::binding::BindingUtil;
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;

use std::collections::VecDeque;
use std::sync::Arc;

pub(crate) struct FilterChainImpl<T: GLInterface> {
    pub(crate) common: FilterCommon,
    passes: Box<[FilterPass<T>]>,
    draw_quad: T::DrawQuad,
    output_framebuffers: Box<[GLFramebuffer]>,
    feedback_framebuffers: Box<[GLFramebuffer]>,
    history_framebuffers: VecDeque<GLFramebuffer>,
    render_target: OutputFramebuffer,
    default_options: FrameOptionsGL,
    draw_last_pass_feedback: bool,
}

pub(crate) struct FilterCommon {
    // semantics: ReflectSemantics,
    pub config: RuntimeParameters,
    pub luts: FastHashMap<usize, InputTexture>,
    pub samplers: SamplerSet,
    pub output_textures: Box<[InputTexture]>,
    pub feedback_textures: Box<[InputTexture]>,
    pub history_textures: Box<[InputTexture]>,
    pub disable_mipmaps: bool,
    pub context: Arc<glow::Context>,
}

impl<T: GLInterface> FilterChainImpl<T> {
    fn reflect_uniform_location(
        ctx: &glow::Context,
        pipeline: glow::Program,
        meta: &dyn UniformMeta,
    ) -> VariableLocation {
        let mut location = VariableLocation {
            ubo: None,
            push: None,
        };

        let offset = meta.offset();

        if offset.ubo.is_some() {
            let vert_name = format!("LIBRA_UBO_VERTEX_INSTANCE.{}", meta.id());
            let frag_name = format!("LIBRA_UBO_FRAGMENT_INSTANCE.{}", meta.id());
            unsafe {
                let vertex = ctx.get_uniform_location(pipeline, &vert_name);
                let fragment = ctx.get_uniform_location(pipeline, &frag_name);
                location.ubo = Some(UniformLocation { vertex, fragment })
            }
        }

        if offset.push.is_some() {
            let vert_name = format!("LIBRA_PUSH_VERTEX_INSTANCE.{}", meta.id());
            let frag_name = format!("LIBRA_PUSH_FRAGMENT_INSTANCE.{}", meta.id());
            unsafe {
                let vertex = ctx.get_uniform_location(pipeline, &vert_name);
                let fragment = ctx.get_uniform_location(pipeline, &frag_name);
                location.push = Some(UniformLocation { vertex, fragment })
            }
        }

        location
    }
}

mod compile {
    use super::*;

    #[cfg(not(feature = "stable"))]
    pub type ShaderPassMeta =
        ShaderPassArtifact<impl CompileReflectShader<GLSL, SpirvCompilation, SpirvCross>>;

    #[cfg(feature = "stable")]
    pub type ShaderPassMeta = ShaderPassArtifact<
        Box<dyn CompileReflectShader<GLSL, SpirvCompilation, SpirvCross> + Send>,
    >;

    pub fn compile_passes(
        shaders: Vec<PassResource>,
        textures: &[TextureResource],
        disable_cache: bool,
    ) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
        let (passes, semantics) = if !disable_cache {
            GLSL::compile_preset_passes::<
                CachedCompilation<SpirvCompilation>,
                SpirvCross,
                FilterChainError,
            >(shaders, textures.iter().map(|t| &t.meta))?
        } else {
            GLSL::compile_preset_passes::<SpirvCompilation, SpirvCross, FilterChainError>(
                shaders,
                textures.iter().map(|t| &t.meta),
            )?
        };

        Ok((passes, semantics))
    }
}

use compile::{compile_passes, ShaderPassMeta};
use librashader_runtime::parameters::RuntimeParameters;

impl<T: GLInterface> FilterChainImpl<T> {
    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub(crate) unsafe fn load_from_pack(
        preset: ShaderPresetPack,
        context: Arc<glow::Context>,
        options: Option<&FilterChainOptionsGL>,
    ) -> error::Result<Self> {
        let disable_cache = options.map_or(false, |o| o.disable_cache);
        let (passes, semantics) = compile_passes(preset.passes, &preset.textures, disable_cache)?;
        let version = options.map_or_else(
            || gl_get_version(&context),
            |o| gl_u16_to_version(&context, o.glsl_version),
        );

        // initialize passes
        let filters = Self::init_passes(&context, version, passes, &semantics, disable_cache)?;

        let default_filter = filters.first().map(|f| f.meta.filter).unwrap_or_default();
        let default_wrap = filters
            .first()
            .map(|f| f.meta.wrap_mode)
            .unwrap_or_default();

        let samplers = SamplerSet::new(&context)?;

        // load luts
        let luts = T::LoadLut::load_luts(&context, preset.textures)?;

        let framebuffer_gen = || T::FramebufferInterface::new(&context, 1);
        let input_gen = || InputTexture {
            image: Default::default(),
            filter: default_filter,
            mip_filter: default_filter,
            wrap_mode: default_wrap,
        };

        let framebuffer_init = FramebufferInit::new(
            filters.iter().map(|f| &f.reflection.meta),
            &framebuffer_gen,
            &input_gen,
        );

        // initialize output framebuffers
        let (output_framebuffers, output_textures) = framebuffer_init.init_output_framebuffers()?;

        // initialize feedback framebuffers
        let (feedback_framebuffers, feedback_textures) =
            framebuffer_init.init_output_framebuffers()?;

        // initialize history
        let (history_framebuffers, history_textures) = framebuffer_init.init_history()?;

        // create vertex objects
        let draw_quad = T::DrawQuad::new(&context)?;

        let output = OutputFramebuffer::new(&context);

        Ok(FilterChainImpl {
            draw_last_pass_feedback: framebuffer_init.uses_final_pass_as_feedback(),
            passes: filters,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            draw_quad,
            common: FilterCommon {
                config: RuntimeParameters::new(preset.pass_count as usize, preset.parameters),
                disable_mipmaps: options.map_or(false, |o| o.force_no_mipmaps),
                luts,
                samplers,
                output_textures,
                feedback_textures,
                history_textures,
                context,
            },
            default_options: Default::default(),
            render_target: output,
        })
    }

    fn init_passes(
        context: &glow::Context,
        version: GlslVersion,
        passes: Vec<ShaderPassMeta>,
        semantics: &ShaderSemantics,
        disable_cache: bool,
    ) -> error::Result<Box<[FilterPass<T>]>> {
        let mut filters = Vec::new();

        // initialize passes
        for (index, (config, mut reflect)) in passes.into_iter().enumerate() {
            let reflection = reflect.reflect(index, semantics)?;
            let glsl = reflect.compile(version)?;

            let (program, ubo_location) =
                T::CompileShader::compile_program(context, glsl, !disable_cache)?;

            let ubo_ring = if let Some(ubo) = &reflection.ubo {
                let ring = T::UboRing::new(&context, ubo.size)?;
                Some(ring)
            } else {
                None
            };

            let uniform_storage = GlUniformStorage::new(
                reflection.ubo.as_ref().map_or(0, |ubo| ubo.size as usize),
                reflection
                    .push_constant
                    .as_ref()
                    .map_or(0, |push| push.size as usize),
            );

            let uniform_bindings = reflection.meta.create_binding_map(|param| {
                UniformOffset::new(
                    Self::reflect_uniform_location(&context, program, param),
                    param.offset(),
                )
            });

            filters.push(FilterPass {
                reflection,
                program,
                ubo_location,
                ubo_ring,
                uniform_storage,
                uniform_bindings,
                source: config.data,
                meta: config.meta,
            });
        }

        Ok(filters.into_boxed_slice())
    }

    fn push_history(&mut self, input: &GLImage) -> error::Result<()> {
        if let Some(mut back) = self.history_framebuffers.pop_back() {
            if back.size != input.size || (input.format != 0 && input.format != back.format) {
                // eprintln!("[history] resizing");
                T::FramebufferInterface::init(&mut back, input.size, input.format)?;
            }

            back.copy_from::<T::FramebufferInterface>(input)?;
            self.history_framebuffers.push_front(back)
        }

        Ok(())
    }

    /// Process a frame with the input image.
    ///
    /// When this frame returns, GL_FRAMEBUFFER is bound to 0.
    pub unsafe fn frame(
        &mut self,
        frame_count: usize,
        viewport: &Viewport<&GLImage>,
        input: &GLImage,
        options: Option<&FrameOptionsGL>,
    ) -> error::Result<()> {
        // limit number of passes to those enabled.
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled());
        let passes = &mut self.passes[0..max];

        if let Some(options) = options {
            if options.clear_history {
                for framebuffer in &self.history_framebuffers {
                    framebuffer.clear::<T::FramebufferInterface, true>()
                }
            }
        }

        if passes.is_empty() {
            return Ok(());
        }
        let options = options.unwrap_or(&self.default_options);

        // do not need to rebind FBO 0 here since first `draw` will
        // bind automatically.
        self.draw_quad
            .bind_vertices(&self.common.context, QuadType::Offscreen);

        let filter = passes[0].meta.filter;
        let wrap_mode = passes[0].meta.wrap_mode;

        // update history
        for (texture, fbo) in self
            .common
            .history_textures
            .iter_mut()
            .zip(self.history_framebuffers.iter())
        {
            texture.image = fbo.as_texture(filter, wrap_mode).image;
        }

        // shader_gl3: 2067
        let original = InputTexture {
            image: *input,
            filter,
            mip_filter: filter,
            wrap_mode,
        };

        let mut source = original;

        // rescale render buffers to ensure all bindings are valid.
        <GLFramebuffer as ScaleFramebuffer<T::FramebufferInterface>>::scale_framebuffers(
            source.image.size,
            viewport.output.size,
            original.image.size,
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
            passes,
            None,
        )?;

        // Refresh inputs for feedback textures.
        // Don't need to do this for outputs because they are yet to be bound.
        for ((texture, fbo), pass) in self
            .common
            .feedback_textures
            .iter_mut()
            .zip(self.feedback_framebuffers.iter())
            .zip(passes.iter())
        {
            texture.image = fbo.as_texture(pass.meta.filter, pass.meta.wrap_mode).image;
        }

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);

        self.draw_quad
            .bind_vertices(&self.common.context, QuadType::Offscreen);
        for (index, pass) in pass.iter_mut().enumerate() {
            let target = &self.output_framebuffers[index];
            source.filter = pass.meta.filter;
            source.mip_filter = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;

            pass.draw(
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                RenderTarget::identity(target)?,
            )?;

            let target = target.as_texture(pass.meta.filter, pass.meta.wrap_mode);
            self.common.output_textures[index] = target;
            source = target;
        }

        self.draw_quad
            .bind_vertices(&self.common.context, QuadType::Final);
        // try to hint the optimizer
        assert_eq!(last.len(), 1);
        if let Some(pass) = last.iter_mut().next() {
            let index = passes_len - 1;
            let final_viewport = self
                .render_target
                .ensure::<T::FramebufferInterface>(viewport.output)?;

            source.filter = pass.meta.filter;
            source.mip_filter = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;

            if self.draw_last_pass_feedback {
                let target = &self.output_framebuffers[index];
                pass.draw(
                    index,
                    &self.common,
                    pass.meta.get_frame_count(frame_count),
                    options,
                    viewport,
                    &original,
                    &source,
                    RenderTarget::viewport_with_output(target, viewport),
                )?;
            }

            pass.draw(
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                RenderTarget::viewport_with_output(final_viewport, viewport),
            )?;
            self.common.output_textures[passes_len - 1] = viewport
                .output
                .as_texture(pass.meta.filter, pass.meta.wrap_mode);
        }

        // swap feedback framebuffers with output
        std::mem::swap(
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
        );

        self.push_history(input)?;

        self.draw_quad.unbind_vertices(&self.common.context);

        Ok(())
    }
}
