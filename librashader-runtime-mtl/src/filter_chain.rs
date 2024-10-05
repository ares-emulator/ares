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
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_presets::context::VideoDriver;
use librashader_presets::ShaderPreset;
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
use librashader_runtime::image::{ImageError, LoadedTexture, UVDirection, BGRA8};
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use librashader_runtime::uniforms::UniformStorage;
use objc2::rc::Id;
use objc2::runtime::ProtocolObject;
use objc2_foundation::NSString;
use objc2_metal::{
    MTLCommandBuffer, MTLCommandEncoder, MTLCommandQueue, MTLDevice, MTLLoadAction, MTLPixelFormat,
    MTLRenderPassDescriptor, MTLResource, MTLStoreAction, MTLTexture,
};
use rayon::prelude::*;
use std::collections::VecDeque;
use std::fmt::{Debug, Formatter};
use std::path::Path;

mod compile {
    use super::*;
    use librashader_pack::{PassResource, TextureResource};

    #[cfg(not(feature = "stable"))]
    pub type ShaderPassMeta =
        ShaderPassArtifact<impl CompileReflectShader<MSL, SpirvCompilation, SpirvCross> + Send>;

    #[cfg(feature = "stable")]
    pub type ShaderPassMeta =
        ShaderPassArtifact<Box<dyn CompileReflectShader<MSL, SpirvCompilation, SpirvCross> + Send>>;

    pub fn compile_passes(
        shaders: Vec<PassResource>,
        textures: &[TextureResource],
    ) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
        let (passes, semantics) = MSL::compile_preset_passes::<
            SpirvCompilation,
            SpirvCross,
            FilterChainError,
        >(shaders, textures.iter().map(|t| &t.meta))?;
        Ok((passes, semantics))
    }
}

use compile::{compile_passes, ShaderPassMeta};
use librashader_pack::{ShaderPresetPack, TextureResource};
use librashader_runtime::parameters::RuntimeParameters;

/// A Metal filter chain.
pub struct FilterChainMetal {
    pub(crate) common: FilterCommon,
    passes: Box<[FilterPass]>,
    output_framebuffers: Box<[OwnedTexture]>,
    feedback_framebuffers: Box<[OwnedTexture]>,
    history_framebuffers: VecDeque<OwnedTexture>,
    /// Metal does not allow us to push the input texture to history
    /// before recording framebuffers, so we double-buffer it.
    ///
    /// First we swap OriginalHistory1 with the contents of this buffer (which were written to
    /// in the previous frame)
    ///
    /// Then we blit the original to the buffer.
    prev_frame_history_buffer: OwnedTexture,
    disable_mipmaps: bool,
    default_options: FrameOptionsMetal,
    draw_last_pass_feedback: bool,
}

impl Debug for FilterChainMetal {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("FilterChainMetal"))
    }
}

pub(crate) struct FilterCommon {
    pub output_textures: Box<[Option<InputTexture>]>,
    pub feedback_textures: Box<[Option<InputTexture>]>,
    pub history_textures: Box<[Option<InputTexture>]>,
    pub luts: FastHashMap<usize, LutTexture>,
    pub samplers: SamplerSet,
    pub config: RuntimeParameters,
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
        let preset = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        Self::load_from_pack(preset, queue, options)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub fn load_from_pack(
        preset: ShaderPresetPack,
        queue: &ProtocolObject<dyn MTLCommandQueue>,
        options: Option<&FilterChainOptionsMetal>,
    ) -> error::Result<FilterChainMetal> {
        let cmd = queue
            .commandBuffer()
            .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;

        let filter_chain =
            Self::load_from_pack_deferred_internal(preset, queue.device(), &cmd, options)?;

        cmd.commit();
        unsafe { cmd.waitUntilCompleted() };

        Ok(filter_chain)
    }

    fn load_luts(
        device: &ProtocolObject<dyn MTLDevice>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        textures: Vec<TextureResource>,
    ) -> error::Result<FastHashMap<usize, LutTexture>> {
        let mut luts = FastHashMap::default();

        let mipmapper = cmd
            .blitCommandEncoder()
            .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;

        let textures = textures
            .into_par_iter()
            .map(|texture| LoadedTexture::<BGRA8>::from_texture(texture, UVDirection::TopLeft))
            .collect::<Result<Vec<LoadedTexture<BGRA8>>, ImageError>>()?;
        for (index, LoadedTexture { meta, image }) in textures.into_iter().enumerate() {
            let texture = LutTexture::new(device, image, &meta, &mipmapper)?;
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
            .map(|(index, (config, mut reflect))| {
                let reflection = reflect.reflect(index, semantics)?;
                let msl = reflect.compile(Some(MslVersion::new(2, 0, 0)))?;

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
                    if let Some(format) = config.meta.get_format_override() {
                        format.into()
                    } else {
                        config.data.format.into()
                    };

                let graphics_pipeline = MetalGraphicsPipeline::new(
                    &device,
                    &msl,
                    if render_pass_format == MTLPixelFormat(0) {
                        MTLPixelFormat::RGBA8Unorm
                    } else {
                        render_pass_format
                    },
                )?;

                Ok(FilterPass {
                    reflection,
                    uniform_storage,
                    uniform_bindings,
                    source: config.data,
                    meta: config.meta,
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
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        input: &ProtocolObject<dyn MTLTexture>,
    ) -> error::Result<()> {
        // If there's no history, there's no need to do any of this.
        let Some(mut back) = self.history_framebuffers.pop_back() else {
            return Ok(());
        };

        // Push the previous frame as OriginalHistory1
        std::mem::swap(&mut back, &mut self.prev_frame_history_buffer);
        self.history_framebuffers.push_front(back);

        // Copy the current frame into prev_frame_history_buffer, which will be
        // pushed to OriginalHistory1 in the next frame.
        let back = &mut self.prev_frame_history_buffer;
        let mipmapper = cmd
            .blitCommandEncoder()
            .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;
        if back.texture.height() != input.height()
            || back.texture.width() != input.width()
            || input.pixelFormat() != back.texture.pixelFormat()
        {
            let size = Size {
                width: input.width() as u32,
                height: input.height() as u32,
            };

            let _old_back = std::mem::replace(
                back,
                OwnedTexture::new(&self.common.device, size, 1, input.pixelFormat())?,
            );
        }

        back.copy_from(&mipmapper, input)?;
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
        let preset = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        Self::load_from_pack_deferred(preset, queue, cmd, options)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command buffer must be ready for recording.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling [`frame`](Self::frame).
    pub fn load_from_pack_deferred(
        preset: ShaderPresetPack,
        queue: &ProtocolObject<dyn MTLCommandQueue>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        options: Option<&FilterChainOptionsMetal>,
    ) -> error::Result<FilterChainMetal> {
        Self::load_from_pack_deferred_internal(preset, queue.device(), &cmd, options)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command buffer must be ready for recording.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling [`frame`](Self::frame).
    fn load_from_pack_deferred_internal(
        preset: ShaderPresetPack,
        device: Id<ProtocolObject<dyn MTLDevice>>,
        cmd: &ProtocolObject<dyn MTLCommandBuffer>,
        options: Option<&FilterChainOptionsMetal>,
    ) -> error::Result<FilterChainMetal> {
        let (passes, semantics) = compile_passes(preset.passes, &preset.textures)?;

        let filters = Self::init_passes(&device, passes, &semantics)?;

        let samplers = SamplerSet::new(&device)?;
        let luts = FilterChainMetal::load_luts(&device, &cmd, preset.textures)?;
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

        let history_buffer = framebuffer_gen()?;

        let draw_quad = DrawQuad::new(&device)?;
        Ok(FilterChainMetal {
            draw_last_pass_feedback: framebuffer_init.uses_final_pass_as_feedback(),
            common: FilterCommon {
                luts,
                samplers,
                config: RuntimeParameters::new(preset.pass_count as usize, preset.parameters),
                draw_quad,
                device,
                output_textures,
                feedback_textures,
                history_textures,
            },
            passes: filters,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            prev_frame_history_buffer: history_buffer,
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
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled());
        if let Some(options) = &options {
            let clear_desc = unsafe { MTLRenderPassDescriptor::new() };
            if options.clear_history {
                for (index, history) in self.history_framebuffers.iter().enumerate() {
                    unsafe {
                        let ca = clear_desc
                            .colorAttachments()
                            .objectAtIndexedSubscript(index);
                        ca.setTexture(Some(&history.texture));
                        ca.setLoadAction(MTLLoadAction::Clear);
                        ca.setStoreAction(MTLStoreAction::Store);
                    }
                }

                let clearpass = cmd
                    .renderCommandEncoderWithDescriptor(&clear_desc)
                    .ok_or(FilterChainError::FailedToCreateCommandBuffer)?;
                clearpass.endEncoding();
            }
        }

        self.push_history(&cmd, &input)?;

        let passes = &mut self.passes[0..max];
        if passes.is_empty() {
            return Ok(());
        }

        let filter = passes[0].meta.filter;
        let wrap_mode = passes[0].meta.wrap_mode;

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
                    Some(feedback.as_input(pass.meta.filter, pass.meta.wrap_mode)?);
                self.common.output_textures[index] =
                    Some(output.as_input(pass.meta.filter, pass.meta.wrap_mode)?);
                Ok(())
            }),
        )?;

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);
        let options = options.unwrap_or(&self.default_options);

        for (index, pass) in pass.iter_mut().enumerate() {
            let target = &self.output_framebuffers[index];
            source.filter_mode = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;
            source.mip_filter = pass.meta.filter;

            let out = RenderTarget::identity(target.texture.as_ref())?;
            pass.draw(
                &cmd,
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
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

            self.common.output_textures[index] =
                Some(target.as_input(pass.meta.filter, pass.meta.wrap_mode)?);
            source = self.common.output_textures[index]
                .as_ref()
                .map(InputTexture::try_clone)
                .unwrap()?;
        }

        // try to hint the optimizer
        assert_eq!(last.len(), 1);

        if let Some(pass) = last.iter_mut().next() {
            if !pass
                .graphics_pipeline
                .has_format(viewport.output.pixelFormat())
            {
                // need to recompile
                pass.graphics_pipeline
                    .recompile(&self.common.device, viewport.output.pixelFormat())?;
            }

            source.filter_mode = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;
            source.mip_filter = pass.meta.filter;
            let index = passes_len - 1;

            if self.draw_last_pass_feedback {
                let output_image = &self.output_framebuffers[index].texture;
                let out = RenderTarget::viewport_with_output(output_image.as_ref(), viewport);
                pass.draw(
                    &cmd,
                    passes_len - 1,
                    &self.common,
                    pass.meta.get_frame_count(frame_count),
                    options,
                    viewport,
                    &original,
                    &source,
                    &out,
                    QuadType::Final,
                )?;
            }

            let out = RenderTarget::viewport(viewport);
            pass.draw(
                &cmd,
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                &out,
                QuadType::Final,
            )?;
        }

        Ok(())
    }
}
