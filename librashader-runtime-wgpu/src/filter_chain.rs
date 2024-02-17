use librashader_common::map::FastHashMap;
use librashader_presets::{ShaderPassConfig, ShaderPreset, TextureConfig};
use librashader_reflect::back::targets::WGSL;
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::SpirvCompilation;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_reflect::reflect::semantics::ShaderSemantics;
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::binding::BindingUtil;
use librashader_runtime::image::{Image, ImageError, UVDirection};
use librashader_runtime::quad::QuadType;
use librashader_runtime::uniforms::UniformStorage;
#[cfg(not(target_arch = "wasm32"))]
use rayon::prelude::*;
use std::collections::VecDeque;
use std::path::Path;

use rayon::ThreadPoolBuilder;
use std::sync::Arc;

use crate::buffer::WgpuStagedBuffer;
use crate::draw_quad::DrawQuad;
use librashader_common::{FilterMode, ImageFormat, Size, Viewport, WrapMode};
use librashader_reflect::reflect::naga::{Naga, NagaLoweringOptions};
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use wgpu::{Device, TextureFormat};

use crate::error;
use crate::error::FilterChainError;
use crate::filter_pass::FilterPass;
use crate::framebuffer::WgpuOutputView;
use crate::graphics_pipeline::WgpuGraphicsPipeline;
use crate::luts::LutTexture;
use crate::mipmap::MipmapGen;
use crate::options::{FilterChainOptionsWgpu, FrameOptionsWgpu};
use crate::samplers::SamplerSet;
use crate::texture::{InputImage, OwnedImage};

type ShaderPassMeta =
    ShaderPassArtifact<impl CompileReflectShader<WGSL, SpirvCompilation, Naga> + Send>;
fn compile_passes(
    shaders: Vec<ShaderPassConfig>,
    textures: &[TextureConfig],
) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
    let (passes, semantics) =
        WGSL::compile_preset_passes::<SpirvCompilation, Naga, FilterChainError>(
            shaders, &textures,
        )?;
    Ok((passes, semantics))
}

/// A wgpu filter chain.
pub struct FilterChainWgpu {
    pub(crate) common: FilterCommon,
    passes: Box<[FilterPass]>,
    output_framebuffers: Box<[OwnedImage]>,
    feedback_framebuffers: Box<[OwnedImage]>,
    history_framebuffers: VecDeque<OwnedImage>,
    disable_mipmaps: bool,
    mipmapper: MipmapGen,
    default_frame_options: FrameOptionsWgpu,
}

pub struct FilterMutable {
    pub passes_enabled: usize,
    pub(crate) parameters: FastHashMap<String, f32>,
}

pub(crate) struct FilterCommon {
    pub output_textures: Box<[Option<InputImage>]>,
    pub feedback_textures: Box<[Option<InputImage>]>,
    pub history_textures: Box<[Option<InputImage>]>,
    pub luts: FastHashMap<usize, LutTexture>,
    pub samplers: SamplerSet,
    pub config: FilterMutable,
    pub internal_frame_count: i32,
    pub(crate) draw_quad: DrawQuad,
    device: Arc<Device>,
    pub(crate) queue: Arc<wgpu::Queue>,
}

impl FilterChainWgpu {
    /// Load the shader preset at the given path into a filter chain.
    pub fn load_from_path(
        path: impl AsRef<Path>,
        device: Arc<Device>,
        queue: Arc<wgpu::Queue>,
        options: Option<&FilterChainOptionsWgpu>,
    ) -> error::Result<FilterChainWgpu> {
        // load passes from preset
        let preset = ShaderPreset::try_parse(path)?;

        Self::load_from_preset(preset, device, queue, options)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub fn load_from_preset(
        preset: ShaderPreset,
        device: Arc<Device>,
        queue: Arc<wgpu::Queue>,
        options: Option<&FilterChainOptionsWgpu>,
    ) -> error::Result<FilterChainWgpu> {
        let mut cmd = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
            label: Some("librashader load cmd"),
        });
        let filter_chain = Self::load_from_preset_deferred(
            preset,
            Arc::clone(&device),
            Arc::clone(&queue),
            &mut cmd,
            options,
        )?;

        let cmd = cmd.finish();

        // Wait for device
        let index = queue.submit([cmd]);
        device.poll(wgpu::Maintain::WaitForSubmissionIndex(index));

        Ok(filter_chain)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command buffer must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling [`frame`](Self::frame).
    pub fn load_from_preset_deferred(
        preset: ShaderPreset,
        device: Arc<Device>,
        queue: Arc<wgpu::Queue>,
        cmd: &mut wgpu::CommandEncoder,
        options: Option<&FilterChainOptionsWgpu>,
    ) -> error::Result<FilterChainWgpu> {
        let (passes, semantics) = compile_passes(preset.shaders, &preset.textures)?;

        // // initialize passes
        let filters = Self::init_passes(Arc::clone(&device), passes, &semantics)?;

        let samplers = SamplerSet::new(&device);
        let mut mipmapper = MipmapGen::new(Arc::clone(&device));
        let luts = FilterChainWgpu::load_luts(
            &device,
            &queue,
            cmd,
            &mut mipmapper,
            &samplers,
            &preset.textures,
        )?;
        //
        let framebuffer_gen = || {
            Ok::<_, error::FilterChainError>(OwnedImage::new(
                Arc::clone(&device),
                Size::new(1, 1),
                1,
                ImageFormat::R8G8B8A8Unorm,
            ))
        };
        let input_gen = || None;
        let framebuffer_init = FramebufferInit::new(
            filters.iter().map(|f| &f.reflection.meta),
            &framebuffer_gen,
            &input_gen,
        );

        //
        // // initialize output framebuffers
        let (output_framebuffers, output_textures) = framebuffer_init.init_output_framebuffers()?;
        //
        // initialize feedback framebuffers
        let (feedback_framebuffers, feedback_textures) =
            framebuffer_init.init_output_framebuffers()?;
        //
        // initialize history
        let (history_framebuffers, history_textures) = framebuffer_init.init_history()?;

        let draw_quad = DrawQuad::new(&device);

        Ok(FilterChainWgpu {
            common: FilterCommon {
                luts,
                samplers,
                config: FilterMutable {
                    passes_enabled: preset.shader_count as usize,
                    parameters: preset
                        .parameters
                        .into_iter()
                        .map(|param| (param.name, param.value))
                        .collect(),
                },
                draw_quad,
                device,
                queue,
                output_textures,
                feedback_textures,
                history_textures,
                internal_frame_count: 0,
            },
            passes: filters,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            disable_mipmaps: options.map(|f| f.force_no_mipmaps).unwrap_or(false),
            mipmapper,
            default_frame_options: Default::default(),
        })
    }

    fn load_luts(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        cmd: &mut wgpu::CommandEncoder,
        mipmapper: &mut MipmapGen,
        sampler_set: &SamplerSet,
        textures: &[TextureConfig],
    ) -> error::Result<FastHashMap<usize, LutTexture>> {
        let mut luts = FastHashMap::default();

        #[cfg(not(target_arch = "wasm32"))]
        let images_iter = textures.par_iter();

        #[cfg(target_arch = "wasm32")]
        let images_iter = textures.iter();

        let images = images_iter
            .map(|texture| Image::load(&texture.path, UVDirection::TopLeft))
            .collect::<Result<Vec<Image>, ImageError>>()?;
        for (index, (texture, image)) in textures.iter().zip(images).enumerate() {
            let texture =
                LutTexture::new(device, queue, cmd, image, texture, mipmapper, sampler_set);
            luts.insert(index, texture);
        }
        Ok(luts)
    }

    fn push_history(&mut self, input: &wgpu::Texture, cmd: &mut wgpu::CommandEncoder) {
        if let Some(mut back) = self.history_framebuffers.pop_back() {
            if back.image.size() != input.size() || input.format() != back.image.format() {
                // old back will get dropped.. do we need to defer?
                let _old_back = std::mem::replace(
                    &mut back,
                    OwnedImage::new(
                        Arc::clone(&self.common.device),
                        input.size().into(),
                        1,
                        input.format().into(),
                    ),
                );
            }

            back.copy_from(cmd, input);

            self.history_framebuffers.push_front(back)
        }
    }

    fn init_passes(
        device: Arc<Device>,
        passes: Vec<ShaderPassMeta>,
        semantics: &ShaderSemantics,
    ) -> error::Result<Box<[FilterPass]>> {
        #[cfg(not(target_arch = "wasm32"))]
        let filter_creation_fn = || {
            let passes_iter = passes.into_par_iter();
            #[cfg(target_arch = "wasm32")]
            let passes_iter = passes.into_iter();

            let filters: Vec<error::Result<FilterPass>> = passes_iter
                .enumerate()
                .map(|(index, (config, source, mut reflect))| {
                    let reflection = reflect.reflect(index, semantics)?;
                    let wgsl = reflect.compile(NagaLoweringOptions {
                        write_pcb_as_ubo: true,
                        sampler_bind_group: 1,
                    })?;

                    let ubo_size = reflection.ubo.as_ref().map_or(0, |ubo| ubo.size as usize);
                    let push_size = reflection
                        .push_constant
                        .as_ref()
                        .map_or(0, |push| push.size as wgpu::BufferAddress);

                    let uniform_storage = UniformStorage::new_with_storage(
                        WgpuStagedBuffer::new(
                            &device,
                            wgpu::BufferUsages::UNIFORM,
                            ubo_size as wgpu::BufferAddress,
                            Some("ubo"),
                        ),
                        WgpuStagedBuffer::new(
                            &device,
                            wgpu::BufferUsages::UNIFORM,
                            push_size as wgpu::BufferAddress,
                            Some("push"),
                        ),
                    );

                    let uniform_bindings =
                        reflection.meta.create_binding_map(|param| param.offset());

                    let render_pass_format: Option<TextureFormat> =
                        if let Some(format) = config.get_format_override() {
                            format.into()
                        } else {
                            source.format.into()
                        };

                    let graphics_pipeline = WgpuGraphicsPipeline::new(
                        Arc::clone(&device),
                        &wgsl,
                        &reflection,
                        render_pass_format.unwrap_or(TextureFormat::Rgba8Unorm),
                    );

                    Ok(FilterPass {
                        device: Arc::clone(&device),
                        reflection,
                        uniform_storage,
                        uniform_bindings,
                        source,
                        config,
                        graphics_pipeline,
                    })
                })
                .collect();
            filters
        };

        #[cfg(target_arch = "wasm32")]
        let filters = filter_creation_fn();

        #[cfg(not(target_arch = "wasm32"))]
        let filters = if let Ok(thread_pool) = ThreadPoolBuilder::new()
            // naga compilations can possibly use degenerate stack sizes.
            .stack_size(10 * 1048576)
            .build()
        {
            thread_pool.install(|| filter_creation_fn())
        } else {
            filter_creation_fn()
        };

        let filters: error::Result<Vec<FilterPass>> = filters.into_iter().collect();
        let filters = filters?;
        Ok(filters.into_boxed_slice())
    }

    /// Records shader rendering commands to the provided command encoder.
    pub fn frame<'a>(
        &mut self,
        input: Arc<wgpu::Texture>,
        viewport: &Viewport<WgpuOutputView<'a>>,
        cmd: &mut wgpu::CommandEncoder,
        frame_count: usize,
        options: Option<&FrameOptionsWgpu>,
    ) -> error::Result<()> {
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled);
        let passes = &mut self.passes[0..max];

        if let Some(options) = &options {
            if options.clear_history {
                for history in &mut self.history_framebuffers {
                    history.clear(cmd);
                }
            }
        }

        if passes.is_empty() {
            return Ok(());
        }

        let original_image_view = input.create_view(&wgpu::TextureViewDescriptor::default());

        let filter = passes[0].config.filter;
        let wrap_mode = passes[0].config.wrap_mode;

        // update history
        for (texture, image) in self
            .common
            .history_textures
            .iter_mut()
            .zip(self.history_framebuffers.iter())
        {
            *texture = Some(image.as_input(filter, wrap_mode));
        }

        let original = InputImage {
            image: Arc::clone(&input),
            view: Arc::new(original_image_view),
            wrap_mode,
            filter_mode: filter,
            mip_filter: filter,
        };

        let mut source = original.clone();

        // swap output and feedback **before** recording command buffers
        std::mem::swap(
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
        );

        // rescale render buffers to ensure all bindings are valid.
        OwnedImage::scale_framebuffers_with_context(
            source.image.size().into(),
            viewport.output.size,
            original.image.size().into(),
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
            passes,
            &(),
            Some(&mut |index: usize,
                       pass: &FilterPass,
                       output: &OwnedImage,
                       feedback: &OwnedImage| {
                // refresh inputs
                self.common.feedback_textures[index] =
                    Some(feedback.as_input(pass.config.filter, pass.config.wrap_mode));
                self.common.output_textures[index] =
                    Some(output.as_input(pass.config.filter, pass.config.wrap_mode));
                Ok(())
            }),
        )?;

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);

        let options = options.unwrap_or(&self.default_frame_options);

        for (index, pass) in pass.iter_mut().enumerate() {
            let target = &self.output_framebuffers[index];
            source.filter_mode = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;
            source.mip_filter = pass.config.filter;

            let output_image = WgpuOutputView::from(target);
            let out = RenderTarget::identity(&output_image);

            pass.draw(
                cmd,
                index,
                &self.common,
                pass.config.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                &out,
                QuadType::Offscreen,
            )?;

            if target.max_miplevels > 1 && !self.disable_mipmaps {
                let sampler = self.common.samplers.get(
                    WrapMode::ClampToEdge,
                    FilterMode::Linear,
                    FilterMode::Nearest,
                );

                target.generate_mipmaps(cmd, &mut self.mipmapper, &sampler);
            }

            source = self.common.output_textures[index].clone().unwrap();
        }

        // try to hint the optimizer
        assert_eq!(last.len(), 1);

        if let Some(pass) = last.iter_mut().next() {
            if pass.graphics_pipeline.format != viewport.output.format {
                // need to recompile
                pass.graphics_pipeline.recompile(viewport.output.format);
            }
            source.filter_mode = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;
            source.mip_filter = pass.config.filter;
            let output_image = &viewport.output;
            let out = RenderTarget::viewport_with_output(output_image, viewport);
            pass.draw(
                cmd,
                passes_len - 1,
                &self.common,
                pass.config.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                &out,
                QuadType::Final,
            )?;
        }

        self.push_history(&input, cmd);
        self.common.internal_frame_count = self.common.internal_frame_count.wrapping_add(1);
        Ok(())
    }
}
