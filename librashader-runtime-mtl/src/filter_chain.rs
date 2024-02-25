use crate::buffer::MetalBuffer;
use crate::draw_quad::DrawQuad;
use crate::error;
use crate::error::FilterChainError;
use crate::filter_pass::FilterPass;
use crate::graphics_pipeline::MetalGraphicsPipeline;
use crate::luts::LutTexture;
use crate::options::{FilterChainOptionsMetal, FrameOptionsMetal};
use crate::samplers::SamplerSet;
use crate::texture::{get_texture_size, InputTexture, MetalTextureRef, OwnedTexture};
use icrate::Foundation::NSString;
use icrate::Metal::{
    MTLCommandBuffer, MTLCommandEncoder, MTLCommandQueue, MTLDevice, MTLLoadActionClear,
    MTLPixelFormat, MTLPixelFormatRGBA8Unorm, MTLRenderPassDescriptor, MTLResource,
    MTLStoreActionDontCare, MTLStoreActionStore, MTLTexture,
};
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_presets::context::VideoDriver;
use librashader_presets::{ShaderPassConfig, ShaderPreset, TextureConfig};
use librashader_reflect::back::msl::MslVersion;
use librashader_reflect::back::targets::MSL;
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::SpirvCompilation;
use librashader_reflect::reflect::cross::SpirvCross;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_reflect::reflect::semantics::ShaderSemantics;
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::binding::BindingUtil;
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::image::{Image, ImageError, UVDirection, BGRA8};
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use librashader_runtime::uniforms::UniformStorage;
use objc2::rc::Id;
use objc2::runtime::ProtocolObject;
use rayon::prelude::*;
use std::collections::VecDeque;
use std::fmt::{Debug, Formatter};
use std::path::Path;

type ShaderPassMeta =
    ShaderPassArtifact<impl CompileReflectShader<MSL, SpirvCompilation, SpirvCross> + Send>;
fn compile_passes(
    shaders: Vec<ShaderPassConfig>,
    textures: &[TextureConfig],
) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
    let (passes, semantics) =
        MSL::compile_preset_passes::<SpirvCompilation, SpirvCross, FilterChainError>(
            shaders, &textures,
        )?;
    Ok((passes, semantics))
}

/// A Metal filter chain.
pub struct FilterChainMetal {
    pub(crate) common: FilterCommon,
    passes: Box<[FilterPass]>,
    output_framebuffers: Box<[OwnedTexture]>,
    feedback_framebuffers: Box<[OwnedTexture]>,
    history_framebuffers: VecDeque<OwnedTexture>,
    disable_mipmaps: bool,
    default_options: FrameOptionsMetal,
}

impl Debug for FilterChainMetal {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("FilterChainMetal"))
    }
}

pub struct FilterMutable {
    pub passes_enabled: usize,
    pub(crate) parameters: FastHashMap<String, f32>,
}

pub(crate) struct FilterCommon {
    pub output_textures: Box<[Option<InputTexture>]>,
    pub feedback_textures: Box<[Option<InputTexture>]>,
    pub history_textures: Box<[Option<InputTexture>]>,
    pub luts: FastHashMap<usize, LutTexture>,
    pub samplers: SamplerSet,
    pub config: FilterMutable,
    pub internal_frame_count: i32,
    pub(crate) draw_quad: DrawQuad,
    device: Id<ProtocolObject<dyn MTLDevice>>,
}

impl FilterChainMetal {
    /// Load the shader preset at the given path into a filter chain.
    pub fn load_from_path(
        path: impl AsRef<Path>,
        queue: &ProtocolObject<dyn MTLCommandQueue>,
        options: Option<&FilterChainOptionsMetal>,
    ) -> error::Result<FilterChainMetal> {
        // load passes from preset
        let preset = ShaderPreset::try_parse_with_driver_context(path, VideoDriver::Metal)?;
        Self::load_from_preset(preset, queue, options)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub fn load_from_preset(
        preset: ShaderPreset,
        queue: &ProtocolObject<dyn MTLCommandQueue>,
        options: Option<&FilterChainOptionsMetal>,
    ) -> error::Result<FilterChainMetal> {
        let cmd = queue
            .commandBuffer()
            .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;

        let filter_chain =
            Self::load_from_preset_deferred_internal(preset, queue.device(), &cmd, options)?;

        cmd.commit();
        unsafe { cmd.waitUntilCompleted() };

        Ok(filter_chain)
    }

    fn load_luts(
        device: &ProtocolObject<dyn MTLDevice>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        textures: &[TextureConfig],
    ) -> error::Result<FastHashMap<usize, LutTexture>> {
        let mut luts = FastHashMap::default();

        let mipmapper = cmd
            .blitCommandEncoder()
            .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;

        let images = textures
            .par_iter()
            .map(|texture| Image::<BGRA8>::load(&texture.path, UVDirection::TopLeft))
            .collect::<Result<Vec<Image<BGRA8>>, ImageError>>()?;
        for (index, (texture, image)) in textures.iter().zip(images).enumerate() {
            let texture = LutTexture::new(device, image, texture, &mipmapper)?;
            luts.insert(index, texture);
        }

        mipmapper.endEncoding();
        Ok(luts)
    }

    fn init_passes(
        device: &Id<ProtocolObject<dyn MTLDevice>>,
        passes: Vec<ShaderPassMeta>,
        semantics: &ShaderSemantics,
    ) -> error::Result<Box<[FilterPass]>> {
        // todo: fix this to allow send
        let filters: Vec<error::Result<FilterPass>> = passes
            .into_iter()
            .enumerate()
            .map(|(index, (config, source, mut reflect))| {
                let reflection = reflect.reflect(index, semantics)?;
                let msl = reflect.compile(Some(MslVersion::V2_0))?;

                let ubo_size = reflection.ubo.as_ref().map_or(0, |ubo| ubo.size as usize);
                let push_size = reflection
                    .push_constant
                    .as_ref()
                    .map_or(0, |push| push.size);

                let uniform_storage = UniformStorage::new_with_storage(
                    MetalBuffer::new(&device, ubo_size, "ubo")?,
                    MetalBuffer::new(&device, push_size as usize, "pcb")?,
                );

                let uniform_bindings = reflection.meta.create_binding_map(|param| param.offset());

                let render_pass_format: MTLPixelFormat =
                    if let Some(format) = config.get_format_override() {
                        format.into()
                    } else {
                        source.format.into()
                    };

                let graphics_pipeline = MetalGraphicsPipeline::new(
                    &device,
                    &msl,
                    if render_pass_format == 0 {
                        MTLPixelFormatRGBA8Unorm
                    } else {
                        render_pass_format
                    },
                )?;

                Ok(FilterPass {
                    reflection,
                    uniform_storage,
                    uniform_bindings,
                    source,
                    config,
                    graphics_pipeline,
                })
            })
            .collect();
        //
        let filters: error::Result<Vec<FilterPass>> = filters.into_iter().collect();
        let filters = filters?;
        Ok(filters.into_boxed_slice())
    }

    fn push_history(
        &mut self,
        input: &ProtocolObject<dyn MTLTexture>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
    ) -> error::Result<()> {
        let mipmapper = cmd
            .blitCommandEncoder()
            .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;
        if let Some(mut back) = self.history_framebuffers.pop_back() {
            if back.texture.height() != input.height()
                || back.texture.width() != input.width()
                || input.pixelFormat() != back.texture.pixelFormat()
            {
                let size = Size {
                    width: input.width() as u32,
                    height: input.height() as u32,
                };

                let _old_back = std::mem::replace(
                    &mut back,
                    OwnedTexture::new(&self.common.device, size, 1, input.pixelFormat())?,
                );
            }

            back.copy_from(&mipmapper, input)?;

            self.history_framebuffers.push_front(back);
        }
        mipmapper.endEncoding();
        Ok(())
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command buffer must be ready for recording.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling [`frame`](Self::frame).
    pub fn load_from_preset_deferred(
        preset: ShaderPreset,
        queue: &ProtocolObject<dyn MTLCommandQueue>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        options: Option<&FilterChainOptionsMetal>,
    ) -> error::Result<FilterChainMetal> {
        Self::load_from_preset_deferred_internal(preset, queue.device(), &cmd, options)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command buffer must be ready for recording.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling [`frame`](Self::frame).
    fn load_from_preset_deferred_internal(
        preset: ShaderPreset,
        device: Id<ProtocolObject<dyn MTLDevice>>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        options: Option<&FilterChainOptionsMetal>,
    ) -> error::Result<FilterChainMetal> {
        let (passes, semantics) = compile_passes(preset.shaders, &preset.textures)?;

        let filters = Self::init_passes(&device, passes, &semantics)?;

        let samplers = SamplerSet::new(&device)?;
        let luts = FilterChainMetal::load_luts(&device, &cmd, &preset.textures)?;
        let framebuffer_gen = || {
            Ok::<_, error::FilterChainError>(OwnedTexture::new(
                &device,
                Size::new(1, 1),
                1,
                ImageFormat::R8G8B8A8Unorm.into(),
            )?)
        };
        let input_gen = || None;
        let framebuffer_init = FramebufferInit::new(
            filters.iter().map(|f| &f.reflection.meta),
            &framebuffer_gen,
            &input_gen,
        );
        let (output_framebuffers, output_textures) = framebuffer_init.init_output_framebuffers()?;
        //
        // initialize feedback framebuffers
        let (feedback_framebuffers, feedback_textures) =
            framebuffer_init.init_output_framebuffers()?;
        //
        // initialize history
        let (history_framebuffers, history_textures) = framebuffer_init.init_history()?;

        let draw_quad = DrawQuad::new(&device)?;
        Ok(FilterChainMetal {
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
            default_options: Default::default(),
        })
    }

    /// Records shader rendering commands to the provided command encoder.
    ///
    /// SAFETY: The `MTLCommandBuffer` provided must not have an active encoder.
    pub fn frame(
        &mut self,
        input: &ProtocolObject<dyn MTLTexture>,
        viewport: &Viewport<MetalTextureRef>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        frame_count: usize,
        options: Option<&FrameOptionsMetal>,
    ) -> error::Result<()> {
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled);
        let passes = &mut self.passes[0..max];
        if let Some(options) = &options {
            let desc = unsafe {
                let desc = MTLRenderPassDescriptor::new();
                desc.colorAttachments()
                    .objectAtIndexedSubscript(0)
                    .setLoadAction(MTLLoadActionClear);

                desc.colorAttachments()
                    .objectAtIndexedSubscript(0)
                    .setStoreAction(MTLStoreActionDontCare);
                desc
            };

            let clear_desc = unsafe { MTLRenderPassDescriptor::new() };
            if options.clear_history {
                for (index, history) in self.history_framebuffers.iter().enumerate() {
                    unsafe {
                        let ca = clear_desc
                            .colorAttachments()
                            .objectAtIndexedSubscript(index);
                        ca.setTexture(Some(&history.texture));
                        ca.setLoadAction(MTLLoadActionClear);
                        ca.setStoreAction(MTLStoreActionStore);
                    }
                }
            }

            let clearpass = cmd
                .renderCommandEncoderWithDescriptor(&desc)
                .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;
            clearpass.endEncoding();
        }
        if passes.is_empty() {
            return Ok(());
        }

        let filter = passes[0].config.filter;
        let wrap_mode = passes[0].config.wrap_mode;

        // update history
        for (texture, image) in self
            .common
            .history_textures
            .iter_mut()
            .zip(self.history_framebuffers.iter())
        {
            *texture = Some(image.as_input(filter, wrap_mode)?);
        }

        let original = InputTexture {
            texture: input
                .newTextureViewWithPixelFormat(input.pixelFormat())
                .ok_or(FilterChainError::FailedToCreateTexture)?,
            wrap_mode,
            filter_mode: filter,
            mip_filter: filter,
        };

        let mut source = original.try_clone()?;

        source
            .texture
            .setLabel(Some(&*NSString::from_str("librashader_sourcetex")));

        // swap output and feedback **before** recording command buffers
        std::mem::swap(
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
        );

        // rescale render buffers to ensure all bindings are valid.
        OwnedTexture::scale_framebuffers_with_context(
            get_texture_size(&source.texture).into(),
            get_texture_size(viewport.output),
            get_texture_size(&original.texture).into(),
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
            passes,
            &self.common.device,
            Some(&mut |index: usize,
                       pass: &FilterPass,
                       output: &OwnedTexture,
                       feedback: &OwnedTexture| {
                // refresh inputs
                self.common.feedback_textures[index] =
                    Some(feedback.as_input(pass.config.filter, pass.config.wrap_mode)?);
                self.common.output_textures[index] =
                    Some(output.as_input(pass.config.filter, pass.config.wrap_mode)?);
                Ok(())
            }),
        )?;

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);
        let options = options.unwrap_or(&self.default_options);

        for (index, pass) in pass.iter_mut().enumerate() {
            let target = &self.output_framebuffers[index];
            source.filter_mode = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;
            source.mip_filter = pass.config.filter;

            let out = RenderTarget::identity(target.texture.as_ref());
            pass.draw(
                &cmd,
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
                target.generate_mipmaps(&cmd)?;
            }

            source = self.common.output_textures[index]
                .as_ref()
                .map(InputTexture::try_clone)
                .unwrap()?;
        }

        // try to hint the optimizer
        assert_eq!(last.len(), 1);

        if let Some(pass) = last.iter_mut().next() {
            if pass.graphics_pipeline.render_pass_format != viewport.output.pixelFormat() {
                // need to recompile
                pass.graphics_pipeline
                    .recompile(&self.common.device, viewport.output.pixelFormat())?;
            }

            source.filter_mode = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;
            source.mip_filter = pass.config.filter;
            let output_image = viewport.output;
            let out = RenderTarget::viewport_with_output(output_image, viewport);
            pass.draw(
                &cmd,
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

        self.push_history(&input, &cmd)?;
        self.common.internal_frame_count = self.common.internal_frame_count.wrapping_add(1);
        Ok(())
    }
}
