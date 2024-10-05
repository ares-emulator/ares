use crate::binding::{ConstantRegister, D3D9UniformBinder, D3D9UniformStorage, RegisterAssignment};
use crate::error;
use crate::filter_chain::FilterCommon;
use crate::options::FrameOptionsD3D9;
use crate::samplers::SamplerSet;
use crate::texture::D3D9InputTexture;
use librashader_common::map::FastHashMap;
use librashader_common::GetSize;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_preprocess::ShaderSource;
use librashader_presets::PassMeta;
use librashader_reflect::reflect::semantics::{TextureBinding, UniformBinding};
use librashader_reflect::reflect::ShaderReflection;
use librashader_runtime::binding::{BindSemantics, UniformInputs};
use librashader_runtime::filter_pass::FilterPassMeta;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use windows::Win32::Foundation::{FALSE, TRUE};

use windows::Win32::Graphics::Direct3D9::{
    IDirect3DDevice9, IDirect3DPixelShader9, IDirect3DSurface9, IDirect3DVertexShader9,
    D3DCLEAR_TARGET, D3DRS_SRGBWRITEENABLE, D3DSAMP_SRGBTEXTURE, D3DVIEWPORT9,
};

pub struct FilterPass {
    pub reflection: ShaderReflection,
    pub vertex_shader: IDirect3DVertexShader9,
    pub pixel_shader: IDirect3DPixelShader9,
    pub uniform_bindings: FastHashMap<UniformBinding, ConstantRegister>,
    pub source: ShaderSource,
    pub meta: PassMeta,
    pub uniform_storage: D3D9UniformStorage,
    pub gl_halfpixel: Option<RegisterAssignment>,
}

impl FilterPassMeta for FilterPass {
    fn framebuffer_format(&self) -> ImageFormat {
        self.source.format
    }

    fn meta(&self) -> &PassMeta {
        &self.meta
    }
}

impl BindSemantics<D3D9UniformBinder, ConstantRegister> for FilterPass {
    type InputTexture = D3D9InputTexture;
    type SamplerSet = SamplerSet;
    type DescriptorSet<'a> = ();
    type DeviceContext = IDirect3DDevice9;
    type UniformOffset = ConstantRegister;

    fn bind_texture<'a>(
        _: &mut Self::DescriptorSet<'a>,
        samplers: &Self::SamplerSet,
        binding: &TextureBinding,
        texture: &Self::InputTexture,
        device: &Self::DeviceContext,
    ) {
        // eprintln!("binding s{}", binding.binding);
        unsafe {
            if let Err(e) = device.SetTexture(binding.binding, &texture.handle) {
                println!(
                    "[librashader-runtime-d3d9] failed to texture at {}: {e}",
                    binding.binding
                );
            }

            let setter = samplers.get(texture.wrap, texture.filter, texture.mipmode);
            if let Err(e) = setter(&device, binding.binding) {
                println!(
                    "[librashader-runtime-d3d9] failed to set sampler at {}: {e}",
                    binding.binding
                );
            }

            if texture.is_srgb {
                if let Err(e) = device.SetSamplerState(binding.binding, D3DSAMP_SRGBTEXTURE, 1u32) {
                    println!(
                        "[librashader-runtime-d3d9] failed to set srgb at {}: {e}",
                        binding.binding
                    );
                }
            } else {
                if let Err(e) = device.SetSamplerState(binding.binding, D3DSAMP_SRGBTEXTURE, 0u32) {
                    println!(
                        "[librashader-runtime-d3d9] failed to set srgb at {}: {e}",
                        binding.binding
                    );
                }
            }
        }
    }
}

impl FilterPass {
    // framecount should be pre-modded
    fn build_semantics<'a>(
        &mut self,
        pass_index: usize,
        parent: &FilterCommon,
        mvp: &[f32; 16],
        frame_count: u32,
        options: &FrameOptionsD3D9,
        fb_size: Size<u32>,
        viewport_size: Size<u32>,
        original: &D3D9InputTexture,
        source: &D3D9InputTexture,
    ) {
        Self::bind_semantics(
            &parent.d3d9,
            &parent.samplers,
            &mut self.uniform_storage,
            &mut (),
            UniformInputs {
                mvp,
                frame_count,
                rotation: options.rotation,
                total_subframes: options.total_subframes,
                current_subframe: options.current_subframe,
                frame_direction: options.frame_direction,
                framebuffer_size: fb_size,
                viewport_size,
            },
            original,
            source,
            &self.uniform_bindings,
            &self.reflection.meta.texture_meta,
            parent.output_textures[0..pass_index]
                .iter()
                .map(|o| o.as_ref()),
            parent.feedback_textures.iter().map(|o| o.as_ref()),
            parent.history_textures.iter().map(|o| o.as_ref()),
            parent.luts.iter().map(|(u, i)| (*u, i.as_ref())),
            &self.source.parameters,
            &parent.config,
        );
    }

    pub(crate) fn draw(
        &mut self,
        device: &IDirect3DDevice9,
        pass_index: usize,
        parent: &FilterCommon,
        frame_count: u32,
        options: &FrameOptionsD3D9,
        viewport: &Viewport<&IDirect3DSurface9>,
        original: &D3D9InputTexture,
        source: &D3D9InputTexture,
        output: RenderTarget<IDirect3DSurface9>,
        vbo_type: QuadType,
    ) -> error::Result<()> {
        if self.meta.mipmap_input && !parent.disable_mipmaps {
            unsafe {
                source.handle.GenerateMipSubLevels();
            }
        }

        let output_size = output.output.size()?;
        // let viewport_size = viewport.output.size()?;
        unsafe {
            device.SetVertexShader(&self.vertex_shader)?;
            device.SetPixelShader(&self.pixel_shader)?;
        }

        self.build_semantics(
            pass_index,
            parent,
            output.mvp,
            frame_count,
            options,
            output_size,
            viewport.output.size()?,
            original,
            source,
        );

        unsafe {
            if let Some(gl_halfpixel) = &self.gl_halfpixel {
                let data = [
                    1.0 / output_size.width as f32,
                    1.0 / output_size.height as f32,
                    0.0,
                    0.0,
                ];
                device.SetVertexShaderConstantF(
                    gl_halfpixel.index,
                    data.as_ptr(),
                    gl_halfpixel.count,
                )?;
            }

            device.SetViewport(&D3DVIEWPORT9 {
                X: output.x as u32,
                Y: output.y as u32,
                Width: output.size.width,
                Height: output.size.height,
                MinZ: 0.0,
                MaxZ: 1.0,
            })?;

            device.SetRenderTarget(0, &*output.output)?;

            device.Clear(
                0,
                std::ptr::null_mut(),
                D3DCLEAR_TARGET as u32,
                if cfg!(debug_assertions) {
                    0xFFFF00FF
                } else {
                    0x0
                },
                0.0,
                0,
            )?;
        }

        if self.framebuffer_format() == ImageFormat::R8G8B8A8Srgb {
            unsafe {
                device.SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE.0 as u32)?;
            }
        }
        parent.draw_quad.draw_quad(device, vbo_type, output.mvp)?;
        unsafe {
            device.SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE.0 as u32)?;
        }
        Ok(())
    }
}
