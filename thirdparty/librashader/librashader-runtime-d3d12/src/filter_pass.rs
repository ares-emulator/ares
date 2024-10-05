use crate::buffer::RawD3D12Buffer;
use crate::descriptor_heap::{ResourceWorkHeap, SamplerWorkHeap};
use crate::error;
use crate::filter_chain::FilterCommon;
use crate::graphics_pipeline::D3D12GraphicsPipeline;
use crate::options::FrameOptionsD3D12;
use crate::samplers::SamplerSet;
use crate::texture::{D3D12OutputView, InputTexture};
use d3d12_descriptor_heap::D3D12DescriptorHeapSlot;
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_preprocess::ShaderSource;
use librashader_presets::PassMeta;
use librashader_reflect::reflect::semantics::{MemberOffset, TextureBinding, UniformBinding};
use librashader_reflect::reflect::ShaderReflection;
use librashader_runtime::binding::{BindSemantics, TextureInput, UniformInputs};
use librashader_runtime::filter_pass::FilterPassMeta;
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::uniforms::{NoUniformBinder, UniformStorage};
use windows::core::Interface;
use windows::Win32::Foundation::RECT;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12GraphicsCommandList, ID3D12GraphicsCommandList4, D3D12_RENDER_PASS_BEGINNING_ACCESS,
    D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD, D3D12_RENDER_PASS_ENDING_ACCESS,
    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, D3D12_RENDER_PASS_FLAG_NONE,
    D3D12_RENDER_PASS_RENDER_TARGET_DESC, D3D12_VIEWPORT,
};

pub(crate) struct FilterPass {
    pub(crate) pipeline: D3D12GraphicsPipeline,
    pub(crate) reflection: ShaderReflection,
    pub(crate) meta: PassMeta,
    pub(crate) uniform_bindings: FastHashMap<UniformBinding, MemberOffset>,
    pub uniform_storage:
        UniformStorage<NoUniformBinder, Option<()>, RawD3D12Buffer, RawD3D12Buffer>,
    pub(crate) texture_heap: [D3D12DescriptorHeapSlot<ResourceWorkHeap>; 16],
    pub(crate) sampler_heap: [D3D12DescriptorHeapSlot<SamplerWorkHeap>; 16],
    pub source: ShaderSource,
}

impl TextureInput for InputTexture {
    fn size(&self) -> Size<u32> {
        self.size
    }
}

impl BindSemantics<NoUniformBinder, Option<()>, RawD3D12Buffer, RawD3D12Buffer> for FilterPass {
    type InputTexture = InputTexture;
    type SamplerSet = SamplerSet;
    type DescriptorSet<'a> = (
        &'a mut [D3D12DescriptorHeapSlot<ResourceWorkHeap>; 16],
        &'a mut [D3D12DescriptorHeapSlot<SamplerWorkHeap>; 16],
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

        unsafe {
            texture_binding[binding.binding as usize].copy_descriptor(*texture.descriptor.as_ref());
            sampler_binding[binding.binding as usize]
                .copy_descriptor(*samplers.get(texture.wrap_mode, texture.filter).as_ref())
        }
    }
}

impl FilterPassMeta for FilterPass {
    fn framebuffer_format(&self) -> ImageFormat {
        self.source.format
    }

    fn meta(&self) -> &PassMeta {
        &self.meta
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
        options: &FrameOptionsD3D12,
        fb_size: Size<u32>,
        viewport_size: Size<u32>,
        original: &InputTexture,
        source: &InputTexture,
    ) {
        Self::bind_semantics(
            &(),
            &parent.samplers,
            &mut self.uniform_storage,
            &mut (&mut self.texture_heap, &mut self.sampler_heap),
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

    /// preconditions
    /// rootsig is bound
    /// descriptor heaps are bound
    /// input must be ready to read from
    /// output must be ready to write to
    pub(crate) fn draw(
        &mut self,
        cmd: &ID3D12GraphicsCommandList,
        pass_index: usize,
        parent: &FilterCommon,
        frame_count: u32,
        options: &FrameOptionsD3D12,
        viewport: &Viewport<D3D12OutputView>,
        original: &InputTexture,
        source: &InputTexture,
        output: &RenderTarget<D3D12OutputView>,
        vbo_type: QuadType,
    ) -> error::Result<()> {
        unsafe {
            cmd.SetPipelineState(self.pipeline.pipeline_state(output.output.format));
        }

        self.build_semantics(
            pass_index,
            parent,
            output.mvp,
            frame_count,
            options,
            output.output.size,
            viewport.output.size,
            original,
            source,
        );

        if self
            .reflection
            .ubo
            .as_ref()
            .is_some_and(|ubo| ubo.size != 0)
        {
            self.uniform_storage.inner_ubo().bind_cbv(2, cmd);
        }

        if self
            .reflection
            .push_constant
            .as_ref()
            .is_some_and(|push| push.size != 0)
        {
            self.uniform_storage.inner_push().bind_cbv(3, cmd);
        }

        unsafe {
            cmd.SetGraphicsRootDescriptorTable(0, *self.texture_heap[0].as_ref());
            cmd.SetGraphicsRootDescriptorTable(1, *self.sampler_heap[0].as_ref());
        }

        // todo: check for non-renderpass.

        let cmd = cmd.cast::<ID3D12GraphicsCommandList4>()?;
        unsafe {
            let pass = [D3D12_RENDER_PASS_RENDER_TARGET_DESC {
                cpuDescriptor: *output.output.descriptor.as_ref(),
                BeginningAccess: D3D12_RENDER_PASS_BEGINNING_ACCESS {
                    Type: D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD,
                    ..Default::default()
                },
                EndingAccess: D3D12_RENDER_PASS_ENDING_ACCESS {
                    Type: D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE,
                    Anonymous: Default::default(),
                },
            }];

            cmd.BeginRenderPass(Some(&pass), None, D3D12_RENDER_PASS_FLAG_NONE)
        }

        unsafe {
            cmd.RSSetViewports(&[D3D12_VIEWPORT {
                TopLeftX: output.x,
                TopLeftY: output.y,
                Width: output.size.width as f32,
                Height: output.size.height as f32,
                MinDepth: 0.0,
                MaxDepth: 1.0,
            }]);

            cmd.RSSetScissorRects(&[RECT {
                left: output.x as i32,
                top: output.y as i32,
                right: output.size.width as i32,
                bottom: output.size.height as i32,
            }]);

            parent.draw_quad.draw_quad(&cmd, vbo_type)
        }

        unsafe { cmd.EndRenderPass() }

        Ok(())
    }
}
