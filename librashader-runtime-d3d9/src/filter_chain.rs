use crate::binding::{update_sampler_bindings, ConstantRegister, RegisterSet};
use crate::draw_quad::DrawQuad;
use crate::error::FilterChainError;
use crate::filter_pass::FilterPass;
use crate::graphics_pipeline::D3D9State;
use crate::luts::LutTexture;
use crate::options::{FilterChainOptionsD3D9, FrameOptionsD3D9};
use crate::samplers::SamplerSet;
use crate::texture::{D3D9InputTexture, D3D9Texture};
use crate::{error, util};
use librashader_cache::{cache_shader_object, CachedCompilation};
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_presets::context::VideoDriver;
use librashader_presets::ShaderPreset;
use librashader_reflect::back::hlsl::HlslShaderModel;
use librashader_reflect::back::targets::HLSL;
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::SpirvCompilation;
use librashader_reflect::reflect::cross::SpirvCross;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_reflect::reflect::semantics::ShaderSemantics;
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::binding::{BindingUtil, TextureInput};
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::image::{ImageError, LoadedTexture, UVDirection, BGRA8};
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use librashader_runtime::uniforms::UniformStorage;
use std::collections::VecDeque;

use librashader_common::GetSize;
use rayon::iter::IntoParallelIterator;
use rayon::iter::ParallelIterator;

use std::path::Path;

use windows::Win32::Graphics::Direct3D9::{IDirect3DDevice9, IDirect3DSurface9, IDirect3DTexture9};

pub(crate) struct FilterCommon {
    pub(crate) d3d9: IDirect3DDevice9,
    pub(crate) luts: FastHashMap<usize, LutTexture>,
    pub samplers: SamplerSet,
    pub output_textures: Box<[Option<D3D9InputTexture>]>,
    pub feedback_textures: Box<[Option<D3D9InputTexture>]>,
    pub history_textures: Box<[Option<D3D9InputTexture>]>,
    pub config: RuntimeParameters,
    pub disable_mipmaps: bool,
    pub(crate) draw_quad: DrawQuad,
}

/// A Direct3D 9 filter chain.
pub struct FilterChainD3D9 {
    pub(crate) common: FilterCommon,
    passes: Vec<FilterPass>,
    output_framebuffers: Box<[D3D9Texture]>,
    feedback_framebuffers: Box<[D3D9Texture]>,
    history_framebuffers: VecDeque<D3D9Texture>,
    default_options: FrameOptionsD3D9,
    draw_last_pass_feedback: bool,
}

mod compile {
    use super::*;
    use librashader_pack::{PassResource, TextureResource};

    #[cfg(not(feature = "stable"))]
    pub type ShaderPassMeta =
        ShaderPassArtifact<impl CompileReflectShader<HLSL, SpirvCompilation, SpirvCross> + Send>;

    #[cfg(feature = "stable")]
    pub type ShaderPassMeta = ShaderPassArtifact<
        Box<dyn CompileReflectShader<HLSL, SpirvCompilation, SpirvCross> + Send>,
    >;

    pub fn compile_passes(
        shaders: Vec<PassResource>,
        textures: &[TextureResource],
        disable_cache: bool,
    ) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
        let (passes, semantics) = if !disable_cache {
            HLSL::compile_preset_passes::<
                CachedCompilation<SpirvCompilation>,
                SpirvCross,
                FilterChainError,
            >(shaders, textures.iter().map(|t| &t.meta))?
        } else {
            HLSL::compile_preset_passes::<SpirvCompilation, SpirvCross, FilterChainError>(
                shaders,
                textures.iter().map(|t| &t.meta),
            )?
        };

        Ok((passes, semantics))
    }
}

use compile::{compile_passes, ShaderPassMeta};
use librashader_pack::{ShaderPresetPack, TextureResource};
use librashader_runtime::parameters::RuntimeParameters;

impl FilterChainD3D9 {
    fn init_passes(
        device: &IDirect3DDevice9,
        passes: Vec<ShaderPassMeta>,
        semantics: &ShaderSemantics,
        disable_cache: bool,
    ) -> error::Result<Vec<FilterPass>> {
        let builder_fn = |(index, (config, mut reflect)): (usize, ShaderPassMeta)| {
            let mut reflection = reflect.reflect(index, semantics)?;
            let hlsl = reflect.compile(Some(HlslShaderModel::ShaderModel3_0))?;

            // eprintln!("===vs===\n{}", hlsl.vertex);

            let (vs, vs_blob) = cache_shader_object(
                "d3d9_sm3",
                &[hlsl.vertex.as_bytes()],
                |&[bytes]| util::d3d_compile_shader(bytes, b"main\0", b"vs_3_0\0"),
                |blob| unsafe {
                    Ok((
                        device.CreateVertexShader(blob.GetBufferPointer().cast())?,
                        blob,
                    ))
                },
                disable_cache,
            )?;

            // eprintln!("===ps===\n{}", hlsl.fragment);

            let (ps, ps_blob) = cache_shader_object(
                "d3d9_sm3",
                &[hlsl.fragment.as_bytes()],
                |&[bytes]| util::d3d_compile_shader(bytes, b"main\0", b"ps_3_0\0"),
                |blob| unsafe {
                    Ok((
                        device.CreatePixelShader(blob.GetBufferPointer().cast())?,
                        blob,
                    ))
                },
                disable_cache,
            )?;

            let uniform_storage = UniformStorage::new(
                reflection.ubo.as_ref().map_or(0, |ubo| ubo.size as usize),
                reflection
                    .push_constant
                    .as_ref()
                    .map_or(0, |push| push.size as usize),
            );

            let mut ps_constants = util::d3d_reflect_shader(ps_blob)?;
            let vs_constants = util::d3d_reflect_shader(vs_blob)?;

            let uniform_bindings = reflection.meta.create_binding_map(|param| {
                ConstantRegister::reflect_register_assignment(
                    param,
                    &ps_constants,
                    &vs_constants,
                    &hlsl.context,
                )
            });

            let gl_halfpixel = vs_constants.get("gl_HalfPixel").map(|o| o.assignment);

            ps_constants.retain(|_, v| matches!(v.set, RegisterSet::Sampler));

            update_sampler_bindings(&mut reflection.meta, &ps_constants);
            // eprintln!("{:?}", ps_constants);
            Ok(FilterPass {
                reflection,
                vertex_shader: vs,
                pixel_shader: ps,
                uniform_bindings,
                uniform_storage,
                gl_halfpixel,
                source: config.data,
                meta: config.meta,
            })
        };

        let filters: Vec<error::Result<FilterPass>> =
            passes.into_iter().enumerate().map(builder_fn).collect();

        let filters: error::Result<Vec<FilterPass>> = filters.into_iter().collect();
        let filters = filters?;
        Ok(filters)
    }

    fn load_luts(
        device: &IDirect3DDevice9,
        textures: Vec<TextureResource>,
    ) -> error::Result<FastHashMap<usize, LutTexture>> {
        let mut luts = FastHashMap::default();
        let images = textures
            .into_par_iter()
            .map(|texture| LoadedTexture::from_texture(texture, UVDirection::TopLeft))
            .collect::<Result<Vec<LoadedTexture<BGRA8>>, ImageError>>()?;

        for (index, LoadedTexture { meta, image }) in images.iter().enumerate() {
            let texture = LutTexture::new(device, &image, &meta)?;
            luts.insert(index, texture);
        }
        Ok(luts)
    }
}

impl FilterChainD3D9 {
    /// Load the shader preset at the given path into a filter chain.
    pub unsafe fn load_from_path(
        path: impl AsRef<Path>,
        device: &IDirect3DDevice9,
        options: Option<&FilterChainOptionsD3D9>,
    ) -> error::Result<FilterChainD3D9> {
        // load passes from preset
        let preset = ShaderPreset::try_parse_with_driver_context(path, VideoDriver::Direct3D11)?;

        unsafe { Self::load_from_preset(preset, device, options) }
    }

    /// Load a filter chain from a pre-parsed and loaded `ShaderPresetPack`.
    pub unsafe fn load_from_preset(
        preset: ShaderPreset,
        device: &IDirect3DDevice9,
        options: Option<&FilterChainOptionsD3D9>,
    ) -> error::Result<FilterChainD3D9> {
        let preset = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        unsafe { Self::load_from_pack(preset, device, options) }
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub unsafe fn load_from_pack(
        preset: ShaderPresetPack,
        device: &IDirect3DDevice9,
        options: Option<&FilterChainOptionsD3D9>,
    ) -> error::Result<FilterChainD3D9> {
        let disable_cache = options.map_or(false, |o| o.disable_cache);

        let (passes, semantics) = compile_passes(preset.passes, &preset.textures, disable_cache)?;

        let samplers = SamplerSet::new()?;

        // initialize passes
        let filters = FilterChainD3D9::init_passes(device, passes, &semantics, disable_cache)?;

        // load luts
        let luts = FilterChainD3D9::load_luts(device, preset.textures)?;

        let framebuffer_gen =
            || D3D9Texture::new(device, Size::new(1, 1), ImageFormat::R8G8B8A8Unorm, false);
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

        Ok(FilterChainD3D9 {
            draw_last_pass_feedback: framebuffer_init.uses_final_pass_as_feedback(),
            passes: filters,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            common: FilterCommon {
                d3d9: device.clone(),
                config: RuntimeParameters::new(preset.pass_count as usize, preset.parameters),
                disable_mipmaps: options.map_or(false, |o| o.force_no_mipmaps),
                luts,
                samplers,
                output_textures,
                feedback_textures,
                history_textures,
                draw_quad,
            },
            default_options: Default::default(),
        })
    }

    fn push_history(&mut self, input: &IDirect3DTexture9) -> error::Result<()> {
        if let Some(mut back) = self.history_framebuffers.pop_back() {
            back.copy_from(&self.common.d3d9, input)?;
            self.history_framebuffers.push_front(back)
        }

        Ok(())
    }

    /// Process a frame with the input image.
    ///
    /// ## Safety:
    ///   * `input` must be in `D3DPOOL_DEFAULT`.
    pub unsafe fn frame(
        &mut self,
        input: &IDirect3DTexture9,
        viewport: &Viewport<&IDirect3DSurface9>,
        frame_count: usize,
        options: Option<&FrameOptionsD3D9>,
    ) -> error::Result<()> {
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled());

        let passes = &mut self.passes[0..max];
        if let Some(options) = options {
            if options.clear_history {
                for framebuffer in &mut self.history_framebuffers {
                    framebuffer.clear(&self.common.d3d9)?;
                }
            }
        }

        if passes.is_empty() {
            return Ok(());
        }

        let options = options.unwrap_or(&self.default_options);
        let filter = passes[0].meta.filter;
        let wrap_mode = passes[0].meta.wrap_mode;

        for (texture, fbo) in self
            .common
            .history_textures
            .iter_mut()
            .zip(self.history_framebuffers.iter())
        {
            *texture = Some(fbo.as_input(filter, filter, wrap_mode));
        }

        let original = D3D9InputTexture {
            handle: input.clone(),
            filter,
            wrap: wrap_mode,
            mipmode: filter,
            is_srgb: false,
        };

        let mut source = original.clone();

        // rescale render buffers to ensure all bindings are valid.
        D3D9Texture::scale_framebuffers(
            source.size(),
            viewport.output.size()?,
            original.size(),
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
            *texture = Some(fbo.as_input(pass.meta.filter, pass.meta.filter, pass.meta.wrap_mode));
        }

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);
        let state_guard = D3D9State::new(&self.common.d3d9)?;

        for (index, pass) in pass.iter_mut().enumerate() {
            source.filter = pass.meta.filter;
            source.wrap = pass.meta.wrap_mode;
            source.is_srgb = pass.meta.srgb_framebuffer;
            let target = &self.output_framebuffers[index];
            let target_rtv = target.as_output()?;
            pass.draw(
                &self.common.d3d9,
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                RenderTarget::identity(&target_rtv)?,
                QuadType::Offscreen,
            )?;

            source = D3D9InputTexture {
                handle: target.handle.clone(),
                filter: pass.meta.filter,
                wrap: pass.meta.wrap_mode,
                mipmode: pass.meta.filter,
                is_srgb: pass.meta.srgb_framebuffer,
            };
            self.common.output_textures[index] = Some(source.clone());
        }

        // try to hint the optimizer
        assert_eq!(last.len(), 1);
        if let Some(pass) = last.iter_mut().next() {
            let index = passes_len - 1;
            source.filter = pass.meta.filter;
            source.wrap = pass.meta.wrap_mode;
            source.is_srgb = pass.meta.srgb_framebuffer;

            if self.draw_last_pass_feedback {
                let feedback_target = &self.output_framebuffers[index];
                let feedback_target_rtv = feedback_target.as_output()?;

                pass.draw(
                    &self.common.d3d9,
                    index,
                    &self.common,
                    pass.meta.get_frame_count(frame_count),
                    options,
                    viewport,
                    &original,
                    &source,
                    RenderTarget::viewport_with_output(&feedback_target_rtv, viewport),
                    QuadType::Final,
                )?;
            }

            pass.draw(
                &self.common.d3d9,
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
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

        self.push_history(&input)?;

        Ok(())
    }
}
