use crate::binding::{GlUniformStorage, UniformLocation, VariableLocation};
use crate::error::FilterChainError;
use crate::filter_pass::{FilterPass, UniformOffset};
use crate::gl::{
    CompileProgram, DrawQuad, FramebufferInterface, GLFramebuffer, GLInterface, LoadLut, UboRing,
};
use crate::options::{FilterChainOptionsGL, FrameOptionsGL};
use crate::samplers::SamplerSet;
use crate::texture::InputTexture;
use crate::util::{gl_get_version, gl_u16_to_version};
use crate::{error, GLImage};
use gl::types::GLuint;
use librashader_common::Viewport;

use librashader_presets::{ShaderPassConfig, ShaderPreset, TextureConfig};
use librashader_reflect::back::glsl::GlslVersion;
use librashader_reflect::back::targets::GLSL;
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::SpirvCompilation;
use librashader_reflect::reflect::semantics::{ShaderSemantics, UniformMeta};

use librashader_cache::CachedCompilation;
use librashader_common::map::FastHashMap;
use librashader_reflect::reflect::cross::SpirvCross;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::binding::BindingUtil;
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use std::collections::VecDeque;

pub(crate) struct FilterChainImpl<T: GLInterface> {
    pub(crate) common: FilterCommon,
    passes: Box<[FilterPass<T>]>,
    draw_quad: T::DrawQuad,
    output_framebuffers: Box<[GLFramebuffer]>,
    feedback_framebuffers: Box<[GLFramebuffer]>,
    history_framebuffers: VecDeque<GLFramebuffer>,
    default_options: FrameOptionsGL,
}

pub(crate) struct FilterCommon {
    // semantics: ReflectSemantics,
    pub config: FilterMutable,
    pub luts: FastHashMap<usize, InputTexture>,
    pub samplers: SamplerSet,
    pub output_textures: Box<[InputTexture]>,
    pub feedback_textures: Box<[InputTexture]>,
    pub history_textures: Box<[InputTexture]>,
    pub disable_mipmaps: bool,
}

pub struct FilterMutable {
    pub(crate) passes_enabled: usize,
    pub(crate) parameters: FastHashMap<String, f32>,
}

impl<T: GLInterface> FilterChainImpl<T> {
    fn reflect_uniform_location(pipeline: GLuint, meta: &dyn UniformMeta) -> VariableLocation {
        let mut location = VariableLocation {
            ubo: None,
            push: None,
        };

        let offset = meta.offset();

        if offset.ubo.is_some() {
            let vert_name = format!("LIBRA_UBO_VERTEX_INSTANCE.{}\0", meta.id());
            let frag_name = format!("LIBRA_UBO_FRAGMENT_INSTANCE.{}\0", meta.id());
            unsafe {
                let vertex = gl::GetUniformLocation(pipeline, vert_name.as_ptr().cast());
                let fragment = gl::GetUniformLocation(pipeline, frag_name.as_ptr().cast());

                location.ubo = Some(UniformLocation { vertex, fragment })
            }
        }

        if offset.push.is_some() {
            let vert_name = format!("LIBRA_PUSH_VERTEX_INSTANCE.{}\0", meta.id());
            let frag_name = format!("LIBRA_PUSH_FRAGMENT_INSTANCE.{}\0", meta.id());
            unsafe {
                let vertex = gl::GetUniformLocation(pipeline, vert_name.as_ptr().cast());
                let fragment = gl::GetUniformLocation(pipeline, frag_name.as_ptr().cast());

                location.push = Some(UniformLocation { vertex, fragment })
            }
        }

        location
    }
}

mod compile {
    use super::*;
    pub type ShaderPassMeta =
        ShaderPassArtifact<impl CompileReflectShader<GLSL, SpirvCompilation, SpirvCross>>;

    pub fn compile_passes(
        shaders: Vec<ShaderPassConfig>,
        textures: &[TextureConfig],
        disable_cache: bool,
    ) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
        let (passes, semantics) = if !disable_cache {
            GLSL::compile_preset_passes::<
                CachedCompilation<SpirvCompilation>,
                SpirvCross,
                FilterChainError,
            >(shaders, &textures)?
        } else {
            GLSL::compile_preset_passes::<SpirvCompilation, SpirvCross, FilterChainError>(
                shaders, &textures,
            )?
        };

        Ok((passes, semantics))
    }
}

use compile::{compile_passes, ShaderPassMeta};

impl<T: GLInterface> FilterChainImpl<T> {
    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub(crate) unsafe fn load_from_preset(
        preset: ShaderPreset,
        options: Option<&FilterChainOptionsGL>,
    ) -> error::Result<Self> {
        let disable_cache = options.map_or(false, |o| o.disable_cache);
        let (passes, semantics) = compile_passes(preset.shaders, &preset.textures, disable_cache)?;
        let version = options.map_or_else(gl_get_version, |o| gl_u16_to_version(o.glsl_version));

        // initialize passes
        let filters = Self::init_passes(version, passes, &semantics, disable_cache)?;

        let default_filter = filters.first().map(|f| f.config.filter).unwrap_or_default();
        let default_wrap = filters
            .first()
            .map(|f| f.config.wrap_mode)
            .unwrap_or_default();

        let samplers = SamplerSet::new();

        // load luts
        let luts = T::LoadLut::load_luts(&preset.textures)?;

        let framebuffer_gen = || Ok::<_, FilterChainError>(T::FramebufferInterface::new(1));
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
        let draw_quad = T::DrawQuad::new();

        Ok(FilterChainImpl {
            passes: filters,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            draw_quad,
            common: FilterCommon {
                config: FilterMutable {
                    passes_enabled: preset.shader_count as usize,
                    parameters: preset
                        .parameters
                        .into_iter()
                        .map(|param| (param.name, param.value))
                        .collect(),
                },
                disable_mipmaps: options.map_or(false, |o| o.force_no_mipmaps),
                luts,
                samplers,
                output_textures,
                feedback_textures,
                history_textures,
            },
            default_options: Default::default(),
        })
    }

    fn init_passes(
        version: GlslVersion,
        passes: Vec<ShaderPassMeta>,
        semantics: &ShaderSemantics,
        disable_cache: bool,
    ) -> error::Result<Box<[FilterPass<T>]>> {
        let mut filters = Vec::new();

        // initialize passes
        for (index, (config, source, mut reflect)) in passes.into_iter().enumerate() {
            let reflection = reflect.reflect(index, semantics)?;
            let glsl = reflect.compile(version)?;

            let (program, ubo_location) = T::CompileShader::compile_program(glsl, !disable_cache)?;

            let ubo_ring = if let Some(ubo) = &reflection.ubo {
                let ring = UboRing::new(ubo.size);
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
                    Self::reflect_uniform_location(program, param),
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
                source,
                config,
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
        viewport: &Viewport<&GLFramebuffer>,
        input: &GLImage,
        options: Option<&FrameOptionsGL>,
    ) -> error::Result<()> {
        // limit number of passes to those enabled.
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled);
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
        self.draw_quad.bind_vertices(QuadType::Offscreen);

        let filter = passes[0].config.filter;
        let wrap_mode = passes[0].config.wrap_mode;

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
            texture.image = fbo
                .as_texture(pass.config.filter, pass.config.wrap_mode)
                .image;
        }

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);

        self.draw_quad.bind_vertices(QuadType::Offscreen);
        for (index, pass) in pass.iter_mut().enumerate() {
            let target = &self.output_framebuffers[index];
            source.filter = pass.config.filter;
            source.mip_filter = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;

            pass.draw(
                index,
                &self.common,
                pass.config.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                RenderTarget::identity(target),
            );

            let target = target.as_texture(pass.config.filter, pass.config.wrap_mode);
            self.common.output_textures[index] = target;
            source = target;
        }

        self.draw_quad.bind_vertices(QuadType::Final);
        // try to hint the optimizer
        assert_eq!(last.len(), 1);
        if let Some(pass) = last.iter_mut().next() {
            source.filter = pass.config.filter;
            source.mip_filter = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;

            pass.draw(
                passes_len - 1,
                &self.common,
                pass.config.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                RenderTarget::viewport(viewport),
            );
            self.common.output_textures[passes_len - 1] = viewport
                .output
                .as_texture(pass.config.filter, pass.config.wrap_mode);
        }

        // swap feedback framebuffers with output
        std::mem::swap(
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
        );

        self.push_history(input)?;

        self.draw_quad.unbind_vertices();

        Ok(())
    }
}
