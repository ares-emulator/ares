use crate::texture::{D3D11InputView, InputTexture, LutTexture};
use librashader_common::{ImageFormat, Size, Viewport};

use librashader_presets::{ShaderPassConfig, ShaderPreset, TextureConfig};
use librashader_reflect::back::targets::HLSL;
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::{Glslang, SpirvCompilation};
use librashader_reflect::reflect::semantics::ShaderSemantics;
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::image::{Image, ImageError, UVDirection};
use rustc_hash::FxHashMap;
use std::collections::VecDeque;

use std::path::Path;

use crate::draw_quad::DrawQuad;
use crate::error::{assume_d3d11_init, FilterChainError};
use crate::filter_pass::{ConstantBufferBinding, FilterPass};
use crate::framebuffer::OwnedImage;
use crate::graphics_pipeline::D3D11State;
use crate::options::{FilterChainOptionsD3D11, FrameOptionsD3D11};
use crate::samplers::SamplerSet;
use crate::util::d3d11_compile_bound_shader;
use crate::{error, util, D3D11OutputView};
use librashader_cache::cache_shader_object;
use librashader_cache::CachedCompilation;
use librashader_presets::context::VideoDriver;
use librashader_reflect::reflect::cross::SpirvCross;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_runtime::binding::{BindingUtil, TextureInput};
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use librashader_runtime::uniforms::UniformStorage;
use rayon::prelude::*;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11Buffer, ID3D11Device, ID3D11DeviceContext, D3D11_BIND_CONSTANT_BUFFER, D3D11_BUFFER_DESC,
    D3D11_CPU_ACCESS_WRITE, D3D11_CREATE_DEVICE_SINGLETHREADED, D3D11_RESOURCE_MISC_FLAG,
    D3D11_RESOURCE_MISC_GENERATE_MIPS, D3D11_TEXTURE2D_DESC, D3D11_USAGE_DEFAULT,
    D3D11_USAGE_DYNAMIC,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R8G8B8A8_UNORM;

pub struct FilterMutable {
    pub(crate) passes_enabled: usize,
    pub(crate) parameters: FxHashMap<String, f32>,
}

/// A Direct3D 11 filter chain.
pub struct FilterChainD3D11 {
    pub(crate) common: FilterCommon,
    passes: Vec<FilterPass>,
    output_framebuffers: Box<[OwnedImage]>,
    feedback_framebuffers: Box<[OwnedImage]>,
    history_framebuffers: VecDeque<OwnedImage>,
    state: D3D11State,
    default_options: FrameOptionsD3D11,
}

pub(crate) struct Direct3D11 {
    pub(crate) _device: ID3D11Device,
    pub(crate) immediate_context: ID3D11DeviceContext,
}

pub(crate) struct FilterCommon {
    pub(crate) d3d11: Direct3D11,
    pub(crate) luts: FxHashMap<usize, LutTexture>,
    pub samplers: SamplerSet,
    pub output_textures: Box<[Option<InputTexture>]>,
    pub feedback_textures: Box<[Option<InputTexture>]>,
    pub history_textures: Box<[Option<InputTexture>]>,
    pub config: FilterMutable,
    pub disable_mipmaps: bool,
    pub(crate) draw_quad: DrawQuad,
}

type ShaderPassMeta =
    ShaderPassArtifact<impl CompileReflectShader<HLSL, SpirvCompilation, SpirvCross> + Send>;
fn compile_passes(
    shaders: Vec<ShaderPassConfig>,
    textures: &[TextureConfig],
    disable_cache: bool,
) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
    let (passes, semantics) = if !disable_cache {
        HLSL::compile_preset_passes::<
            Glslang,
            CachedCompilation<SpirvCompilation>,
            SpirvCross,
            FilterChainError,
        >(shaders, &textures)?
    } else {
        HLSL::compile_preset_passes::<Glslang, SpirvCompilation, SpirvCross, FilterChainError>(
            shaders, &textures,
        )?
    };

    Ok((passes, semantics))
}

impl FilterChainD3D11 {
    /// Load the shader preset at the given path into a filter chain.
    pub unsafe fn load_from_path(
        path: impl AsRef<Path>,
        device: &ID3D11Device,
        options: Option<&FilterChainOptionsD3D11>,
    ) -> error::Result<FilterChainD3D11> {
        // load passes from preset
        let preset = ShaderPreset::try_parse_with_driver_context(path, VideoDriver::Direct3D11)?;

        unsafe { Self::load_from_preset(preset, device, options) }
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub unsafe fn load_from_preset(
        preset: ShaderPreset,
        device: &ID3D11Device,
        options: Option<&FilterChainOptionsD3D11>,
    ) -> error::Result<FilterChainD3D11> {
        let immediate_context = unsafe { device.GetImmediateContext()? };
        unsafe { Self::load_from_preset_deferred(preset, device, &immediate_context, options) }
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function is therefore requires no external synchronization of the
    /// immediate context, as long as the immediate context is not used as the input context,
    /// nor of the device, as long as the device is not single-threaded only.
    ///
    /// ## Safety
    /// The provided context must either be immediate, or immediately submitted after this function
    /// returns, **before drawing frames**, or lookup textures will fail to load and the filter chain
    /// will be in an invalid state.
    ///
    /// If the context is deferred, it must be ready for command recording, and have no prior commands
    /// recorded. No commands shall be recorded after, the caller must immediately call [`FinishCommandList`](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-finishcommandlist)
    /// and execute the command list on the immediate context after this function returns.
    ///
    /// If the context is immediate, then access to the immediate context requires external synchronization.
    pub unsafe fn load_from_preset_deferred(
        preset: ShaderPreset,
        device: &ID3D11Device,
        ctx: &ID3D11DeviceContext,
        options: Option<&FilterChainOptionsD3D11>,
    ) -> error::Result<FilterChainD3D11> {
        let disable_cache = options.map_or(false, |o| o.disable_cache);

        let (passes, semantics) = compile_passes(preset.shaders, &preset.textures, disable_cache)?;

        let samplers = SamplerSet::new(device)?;

        // initialize passes
        let filters = FilterChainD3D11::init_passes(device, passes, &semantics, disable_cache)?;

        let immediate_context = unsafe { device.GetImmediateContext()? };

        // load luts
        let luts = FilterChainD3D11::load_luts(device, &ctx, &preset.textures)?;

        let framebuffer_gen =
            || OwnedImage::new(device, Size::new(1, 1), ImageFormat::R8G8B8A8Unorm, false);
        let input_gen = || None;
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

        let draw_quad = DrawQuad::new(device)?;
        let state = D3D11State::new(device)?;
        Ok(FilterChainD3D11 {
            passes: filters,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            common: FilterCommon {
                d3d11: Direct3D11 {
                    _device: device.clone(),
                    immediate_context,
                },
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
                draw_quad,
            },
            state,
            default_options: Default::default(),
        })
    }
}

impl FilterChainD3D11 {
    fn create_constant_buffer(device: &ID3D11Device, size: u32) -> error::Result<ID3D11Buffer> {
        unsafe {
            let mut buffer = None;
            device.CreateBuffer(
                &D3D11_BUFFER_DESC {
                    ByteWidth: size,
                    Usage: D3D11_USAGE_DYNAMIC,
                    BindFlags: D3D11_BIND_CONSTANT_BUFFER,
                    CPUAccessFlags: D3D11_CPU_ACCESS_WRITE,
                    MiscFlags: D3D11_RESOURCE_MISC_FLAG(0),
                    StructureByteStride: 0,
                },
                None,
                Some(&mut buffer),
            )?;
            assume_d3d11_init!(buffer, "CreateBuffer");
            Ok(buffer)
        }
    }

    fn init_passes(
        device: &ID3D11Device,
        passes: Vec<ShaderPassMeta>,
        semantics: &ShaderSemantics,
        disable_cache: bool,
    ) -> error::Result<Vec<FilterPass>> {
        let device_is_singlethreaded =
            unsafe { (device.GetCreationFlags() & D3D11_CREATE_DEVICE_SINGLETHREADED.0) == 1 };

        let builder_fn = |(index, (config, source, mut reflect)): (usize, ShaderPassMeta)| {
            let reflection = reflect.reflect(index, semantics)?;
            let hlsl = reflect.compile(None)?;

            let (vs, vertex_dxbc) = cache_shader_object(
                "dxbc",
                &[hlsl.vertex.as_bytes()],
                |&[bytes]| util::d3d_compile_shader(bytes, b"main\0", b"vs_5_0\0"),
                |blob| {
                    Ok((
                        d3d11_compile_bound_shader(
                            device,
                            &blob,
                            None,
                            ID3D11Device::CreateVertexShader,
                        )?,
                        blob,
                    ))
                },
                disable_cache,
            )?;

            let ia_desc = DrawQuad::get_spirv_cross_vbo_desc();
            let vao = util::d3d11_create_input_layout(device, &ia_desc, &vertex_dxbc)?;

            let ps = cache_shader_object(
                "dxbc",
                &[hlsl.fragment.as_bytes()],
                |&[bytes]| util::d3d_compile_shader(bytes, b"main\0", b"ps_5_0\0"),
                |blob| {
                    d3d11_compile_bound_shader(device, &blob, None, ID3D11Device::CreatePixelShader)
                },
                disable_cache,
            )?;

            let ubo_cbuffer = if let Some(ubo) = &reflection.ubo
                && ubo.size != 0
            {
                let buffer = FilterChainD3D11::create_constant_buffer(device, ubo.size)?;
                Some(ConstantBufferBinding {
                    binding: ubo.binding,
                    size: ubo.size,
                    stage_mask: ubo.stage_mask,
                    buffer,
                })
            } else {
                None
            };

            let push_cbuffer = if let Some(push) = &reflection.push_constant
                && push.size != 0
            {
                let buffer = FilterChainD3D11::create_constant_buffer(device, push.size)?;
                Some(ConstantBufferBinding {
                    binding: if ubo_cbuffer.is_some() { 1 } else { 0 },
                    size: push.size,
                    stage_mask: push.stage_mask,
                    buffer,
                })
            } else {
                None
            };

            let uniform_storage = UniformStorage::new(
                reflection.ubo.as_ref().map_or(0, |ubo| ubo.size as usize),
                reflection
                    .push_constant
                    .as_ref()
                    .map_or(0, |push| push.size as usize),
            );

            let uniform_bindings = reflection.meta.create_binding_map(|param| param.offset());

            Ok(FilterPass {
                reflection,
                vertex_shader: vs,
                vertex_layout: vao,
                pixel_shader: ps,
                uniform_bindings,
                uniform_storage,
                uniform_buffer: ubo_cbuffer,
                push_buffer: push_cbuffer,
                source,
                config,
            })
        };

        let filters: Vec<error::Result<FilterPass>> = if device_is_singlethreaded {
            // D3D11Device is not thread safe
            passes.into_iter().enumerate().map(builder_fn).collect()
        } else {
            // D3D11Device is thread safe
            passes.into_par_iter().enumerate().map(builder_fn).collect()
        };

        let filters: error::Result<Vec<FilterPass>> = filters.into_iter().collect();
        let filters = filters?;
        Ok(filters)
    }

    fn push_history(
        &mut self,
        ctx: &ID3D11DeviceContext,
        input: &D3D11InputView,
    ) -> error::Result<()> {
        if let Some(mut back) = self.history_framebuffers.pop_back() {
            back.copy_from(ctx, input)?;
            self.history_framebuffers.push_front(back)
        }

        Ok(())
    }

    fn load_luts(
        device: &ID3D11Device,
        context: &ID3D11DeviceContext,
        textures: &[TextureConfig],
    ) -> error::Result<FxHashMap<usize, LutTexture>> {
        let mut luts = FxHashMap::default();
        let images = textures
            .par_iter()
            .map(|texture| Image::load(&texture.path, UVDirection::TopLeft))
            .collect::<Result<Vec<Image>, ImageError>>()?;

        for (index, (texture, image)) in textures.iter().zip(images).enumerate() {
            let desc = D3D11_TEXTURE2D_DESC {
                Width: image.size.width,
                Height: image.size.height,
                Format: DXGI_FORMAT_R8G8B8A8_UNORM,
                Usage: D3D11_USAGE_DEFAULT,
                MiscFlags: if texture.mipmap {
                    D3D11_RESOURCE_MISC_GENERATE_MIPS
                } else {
                    D3D11_RESOURCE_MISC_FLAG(0)
                },
                ..Default::default()
            };

            let texture = LutTexture::new(
                device,
                context,
                &image,
                desc,
                texture.filter_mode,
                texture.wrap_mode,
            )?;
            luts.insert(index, texture);
        }
        Ok(luts)
    }

    /// Process a frame with the input image.
    pub unsafe fn frame(
        &mut self,
        ctx: Option<&ID3D11DeviceContext>,
        input: D3D11InputView,
        viewport: &Viewport<D3D11OutputView>,
        frame_count: usize,
        options: Option<&FrameOptionsD3D11>,
    ) -> error::Result<()> {
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled);

        // Need to clone this because pushing history needs a mutable borrow.
        let immediate_context = &self.common.d3d11.immediate_context.clone();
        let ctx = ctx.unwrap_or(immediate_context);

        let passes = &mut self.passes[0..max];
        if let Some(options) = options {
            if options.clear_history {
                for framebuffer in &mut self.history_framebuffers {
                    framebuffer.clear(ctx)?;
                }
            }
        }

        if passes.is_empty() {
            return Ok(());
        }

        let options = options.unwrap_or(&self.default_options);
        let filter = passes[0].config.filter;
        let wrap_mode = passes[0].config.wrap_mode;

        for (texture, fbo) in self
            .common
            .history_textures
            .iter_mut()
            .zip(self.history_framebuffers.iter())
        {
            *texture = Some(InputTexture::from_framebuffer(fbo, wrap_mode, filter)?);
        }

        let original = InputTexture {
            view: input.clone(),
            filter,
            wrap_mode,
        };

        let mut source = original.clone();

        // rescale render buffers to ensure all bindings are valid.
        OwnedImage::scale_framebuffers(
            source.size(),
            viewport.output.size,
            original.view.size,
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
            *texture = Some(InputTexture::from_framebuffer(
                fbo,
                pass.config.wrap_mode,
                pass.config.filter,
            )?);
        }

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);

        let state_guard = self.state.enter_filter_state(ctx);
        self.common.draw_quad.bind_vbo_for_frame(ctx);

        for (index, pass) in pass.iter_mut().enumerate() {
            source.filter = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;
            let target = &self.output_framebuffers[index];
            let size = target.size;
            pass.draw(
                ctx,
                index,
                &self.common,
                pass.config.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                RenderTarget::identity(&target.as_output()?),
                QuadType::Offscreen,
            )?;

            source = InputTexture {
                view: D3D11InputView {
                    handle: target.create_shader_resource_view()?,
                    size,
                },
                filter: pass.config.filter,
                wrap_mode: pass.config.wrap_mode,
            };
            self.common.output_textures[index] = Some(source.clone());
        }

        // try to hint the optimizer
        assert_eq!(last.len(), 1);
        if let Some(pass) = last.iter_mut().next() {
            source.filter = pass.config.filter;
            source.wrap_mode = pass.config.wrap_mode;
            pass.draw(
                &ctx,
                passes_len - 1,
                &self.common,
                pass.config.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                RenderTarget::viewport(viewport),
                QuadType::Final,
            )?;
        }

        std::mem::swap(
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
        );

        drop(state_guard);

        self.push_history(ctx, &input)?;

        Ok(())
    }
}
