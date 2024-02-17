use crate::filter_chain::FilterCommon;
use crate::options::FrameOptionsD3D11;
use crate::texture::InputTexture;

use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_preprocess::ShaderSource;
use librashader_presets::ShaderPassConfig;
use librashader_reflect::reflect::semantics::{
    BindingStage, MemberOffset, TextureBinding, UniformBinding,
};
use librashader_reflect::reflect::ShaderReflection;

use librashader_runtime::binding::{BindSemantics, TextureInput, UniformInputs};
use librashader_runtime::filter_pass::FilterPassMeta;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11Buffer, ID3D11DeviceContext, ID3D11InputLayout, ID3D11PixelShader, ID3D11SamplerState,
    ID3D11ShaderResourceView, ID3D11VertexShader, D3D11_MAPPED_SUBRESOURCE,
    D3D11_MAP_WRITE_DISCARD, D3D11_VIEWPORT,
};

use crate::samplers::SamplerSet;
use crate::{error, D3D11OutputView};
use librashader_runtime::uniforms::{UniformStorage, UniformStorageAccess};

pub struct ConstantBufferBinding {
    pub binding: u32,
    pub size: u32,
    pub stage_mask: BindingStage,
    pub buffer: ID3D11Buffer,
}

// slang_process.cpp 141
pub struct FilterPass {
    pub reflection: ShaderReflection,
    pub vertex_shader: ID3D11VertexShader,
    pub vertex_layout: ID3D11InputLayout,
    pub pixel_shader: ID3D11PixelShader,

    pub uniform_bindings: FastHashMap<UniformBinding, MemberOffset>,

    pub uniform_storage: UniformStorage,
    pub uniform_buffer: Option<ConstantBufferBinding>,
    pub push_buffer: Option<ConstantBufferBinding>,
    pub source: ShaderSource,
    pub config: ShaderPassConfig,
}

// https://doc.rust-lang.org/nightly/core/array/fn.from_fn.html is not ~const :(
const NULL_TEXTURES: &[Option<ID3D11ShaderResourceView>; 16] = &[
    None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None,
];

impl TextureInput for InputTexture {
    fn size(&self) -> Size<u32> {
        self.view.size
    }
}

impl BindSemantics for FilterPass {
    type InputTexture = InputTexture;
    type SamplerSet = SamplerSet;
    type DescriptorSet<'a> = (
        &'a mut [Option<ID3D11ShaderResourceView>; 16],
        &'a mut [Option<ID3D11SamplerState>; 16],
    );
    type DeviceContext = ();
    type UniformOffset = MemberOffset;

    fn bind_texture<'a>(
        descriptors: &mut Self::DescriptorSet<'a>,
        samplers: &Self::SamplerSet,
        binding: &TextureBinding,
        texture: &Self::InputTexture,
        _device: &Self::DeviceContext,
    ) {
        let (texture_binding, sampler_binding) = descriptors;
        texture_binding[binding.binding as usize] = Some(texture.view.handle.clone());
        sampler_binding[binding.binding as usize] =
            Some(samplers.get(texture.wrap_mode, texture.filter).clone());
    }
}

impl FilterPassMeta for FilterPass {
    fn framebuffer_format(&self) -> ImageFormat {
        self.source.format
    }

    fn config(&self) -> &ShaderPassConfig {
        &self.config
    }
}

// slang_process.cpp 229
impl FilterPass {
    // framecount should be pre-modded
    fn build_semantics<'a>(
        &mut self,
        pass_index: usize,
        parent: &FilterCommon,
        mvp: &[f32; 16],
        frame_count: u32,
        options: &FrameOptionsD3D11,
        fb_size: Size<u32>,
        viewport_size: Size<u32>,
        mut descriptors: (
            &'a mut [Option<ID3D11ShaderResourceView>; 16],
            &'a mut [Option<ID3D11SamplerState>; 16],
        ),
        original: &InputTexture,
        source: &InputTexture,
    ) {
        Self::bind_semantics(
            &(),
            &parent.samplers,
            &mut self.uniform_storage,
            &mut descriptors,
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
            &parent.config.parameters,
        );
    }

    pub(crate) fn draw(
        &mut self,
        ctx: &ID3D11DeviceContext,
        pass_index: usize,
        parent: &FilterCommon,
        frame_count: u32,
        options: &FrameOptionsD3D11,
        viewport: &Viewport<D3D11OutputView>,
        original: &InputTexture,
        source: &InputTexture,
        output: RenderTarget<D3D11OutputView>,
        vbo_type: QuadType,
    ) -> error::Result<()> {
        if self.config.mipmap_input && !parent.disable_mipmaps {
            unsafe {
                ctx.GenerateMips(&source.view.handle);
            }
        }
        unsafe {
            ctx.IASetInputLayout(&self.vertex_layout);
            ctx.VSSetShader(&self.vertex_shader, None);
            ctx.PSSetShader(&self.pixel_shader, None);
        }

        let mut textures: [Option<ID3D11ShaderResourceView>; 16] = std::array::from_fn(|_| None);
        let mut samplers: [Option<ID3D11SamplerState>; 16] = std::array::from_fn(|_| None);
        let descriptors = (&mut textures, &mut samplers);

        self.build_semantics(
            pass_index,
            parent,
            output.mvp,
            frame_count,
            options,
            output.output.size,
            viewport.output.size,
            descriptors,
            original,
            source,
        );

        if let Some(ubo) = &self.uniform_buffer {
            // upload uniforms
            unsafe {
                let mut map = D3D11_MAPPED_SUBRESOURCE::default();
                ctx.Map(&ubo.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, Some(&mut map))?;
                std::ptr::copy_nonoverlapping(
                    self.uniform_storage.ubo_pointer(),
                    map.pData.cast(),
                    ubo.size as usize,
                );
                ctx.Unmap(&ubo.buffer, 0);
            }

            if ubo.stage_mask.contains(BindingStage::VERTEX) {
                unsafe { ctx.VSSetConstantBuffers(ubo.binding, Some(&[Some(ubo.buffer.clone())])) }
            }
            if ubo.stage_mask.contains(BindingStage::FRAGMENT) {
                unsafe { ctx.PSSetConstantBuffers(ubo.binding, Some(&[Some(ubo.buffer.clone())])) }
            }
        }

        if let Some(push) = &self.push_buffer {
            // upload push constants
            unsafe {
                let mut map = D3D11_MAPPED_SUBRESOURCE::default();
                ctx.Map(&push.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, Some(&mut map))?;
                std::ptr::copy_nonoverlapping(
                    self.uniform_storage.push_pointer(),
                    map.pData.cast(),
                    push.size as usize,
                );
                ctx.Unmap(&push.buffer, 0);
            }

            if push.stage_mask.contains(BindingStage::VERTEX) {
                unsafe {
                    ctx.VSSetConstantBuffers(push.binding, Some(&[Some(push.buffer.clone())]))
                }
            }
            if push.stage_mask.contains(BindingStage::FRAGMENT) {
                unsafe {
                    ctx.PSSetConstantBuffers(push.binding, Some(&[Some(push.buffer.clone())]))
                }
            }
        }

        unsafe {
            // reset RTVs
            ctx.OMSetRenderTargets(None, None);
        }

        unsafe {
            // SAFETY: Niche optimization for Option<NonNull<T>>
            // Assumes that IUnknown is defined as IUnknown(std::ptr::NonNull<std::ffi::c_void>)
            const _: () = assert!(
                std::mem::size_of::<Option<windows::core::IUnknown>>()
                    == std::mem::size_of::<windows::core::IUnknown>()
            );
            ctx.PSSetShaderResources(0, Some(std::mem::transmute(textures.as_ref())));
            ctx.PSSetSamplers(0, Some(std::mem::transmute(samplers.as_ref())));

            ctx.OMSetRenderTargets(Some(&[Some(output.output.handle.clone())]), None);
            ctx.RSSetViewports(Some(&[D3D11_VIEWPORT {
                TopLeftX: output.x,
                TopLeftY: output.y,
                Width: output.output.size.width as f32,
                Height: output.output.size.height as f32,
                MinDepth: 0.0,
                MaxDepth: 1.0,
            }]))
        }

        parent.draw_quad.draw_quad(ctx, vbo_type);

        unsafe {
            // unbind resources.
            ctx.PSSetShaderResources(0, Some(std::mem::transmute(NULL_TEXTURES.as_ref())));
            ctx.OMSetRenderTargets(None, None);
        }
        Ok(())
    }
}
