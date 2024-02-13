use crate::framebuffer::WgpuOutputView;
use crate::util;
use librashader_reflect::back::wgsl::NagaWgslContext;
use librashader_reflect::back::ShaderCompilerOutput;
use librashader_reflect::reflect::ShaderReflection;
use librashader_runtime::render_target::RenderTarget;
use std::borrow::Cow;
use std::sync::Arc;
use wgpu::{
    BindGroupLayout, BindGroupLayoutDescriptor, BindGroupLayoutEntry, BindingType,
    BufferBindingType, BufferSize, CommandEncoder, Operations, PipelineLayout, PushConstantRange,
    RenderPass, RenderPassColorAttachment, RenderPassDescriptor, SamplerBindingType, ShaderModule,
    ShaderSource, ShaderStages, TextureFormat, TextureSampleType, TextureViewDimension,
    VertexBufferLayout,
};

pub struct WgpuGraphicsPipeline {
    pub layout: PipelineLayoutObjects,
    render_pipeline: wgpu::RenderPipeline,
    pub format: wgpu::TextureFormat,
}

pub struct PipelineLayoutObjects {
    layout: PipelineLayout,
    pub main_bind_group_layout: BindGroupLayout,
    pub sampler_bind_group_layout: BindGroupLayout,
    fragment_entry_name: String,
    vertex_entry_name: String,
    vertex: ShaderModule,
    fragment: ShaderModule,
    device: Arc<wgpu::Device>,
}

impl PipelineLayoutObjects {
    pub fn new(
        reflection: &ShaderReflection,
        shader_assembly: &ShaderCompilerOutput<String, NagaWgslContext>,
        device: Arc<wgpu::Device>,
    ) -> Self {
        let vertex = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("vertex"),
            source: ShaderSource::Wgsl(Cow::from(&shader_assembly.vertex)),
        });

        let fragment = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("fragment"),
            source: ShaderSource::Wgsl(Cow::from(&shader_assembly.fragment)),
        });

        let mut main_bindings = Vec::new();
        let mut sampler_bindings = Vec::new();

        let mut push_constant_range = Vec::new();

        if let Some(push_meta) = reflection.push_constant.as_ref()
            && !push_meta.stage_mask.is_empty()
        {
            let push_mask = util::binding_stage_to_wgpu_stage(push_meta.stage_mask);

            if let Some(binding) = push_meta.binding {
                main_bindings.push(BindGroupLayoutEntry {
                    binding,
                    visibility: push_mask,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: BufferSize::new(push_meta.size as u64),
                    },
                    count: None,
                });
            } else {
                push_constant_range.push(PushConstantRange {
                    stages: push_mask,
                    range: 0..push_meta.size,
                })
            }
        }

        if let Some(ubo_meta) = reflection.ubo.as_ref()
            && !ubo_meta.stage_mask.is_empty()
        {
            let ubo_mask = util::binding_stage_to_wgpu_stage(ubo_meta.stage_mask);
            main_bindings.push(BindGroupLayoutEntry {
                binding: ubo_meta.binding,
                visibility: ubo_mask,
                ty: BindingType::Buffer {
                    ty: BufferBindingType::Uniform,
                    has_dynamic_offset: false,
                    min_binding_size: BufferSize::new(ubo_meta.size as u64),
                },
                count: None,
            });
        }

        for texture in reflection.meta.texture_meta.values() {
            main_bindings.push(BindGroupLayoutEntry {
                binding: texture.binding,
                visibility: ShaderStages::FRAGMENT,
                ty: BindingType::Texture {
                    sample_type: TextureSampleType::Float { filterable: true },
                    view_dimension: TextureViewDimension::D2,
                    multisampled: false,
                },
                count: None,
            });

            sampler_bindings.push(BindGroupLayoutEntry {
                binding: texture.binding,
                visibility: ShaderStages::FRAGMENT,
                ty: BindingType::Sampler(SamplerBindingType::Filtering),
                count: None,
            })
        }
        let main_bind_group = device.create_bind_group_layout(&BindGroupLayoutDescriptor {
            label: Some("bind group 0"),
            entries: &main_bindings,
        });

        let sampler_bind_group = device.create_bind_group_layout(&BindGroupLayoutDescriptor {
            label: Some("bind group 1"),
            entries: &sampler_bindings,
        });

        let bind_group_layout_refs = [&main_bind_group, &sampler_bind_group];

        let layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("shader pipeline layout"),
            bind_group_layouts: &bind_group_layout_refs,
            push_constant_ranges: &push_constant_range.as_ref(),
        });

        Self {
            layout,
            main_bind_group_layout: main_bind_group,
            sampler_bind_group_layout: sampler_bind_group,
            fragment_entry_name: shader_assembly.context.fragment.entry_points[0]
                .name
                .clone(),
            vertex_entry_name: shader_assembly.context.vertex.entry_points[0].name.clone(),
            vertex,
            fragment,
            device,
        }
    }

    pub fn create_pipeline(&self, framebuffer_format: TextureFormat) -> wgpu::RenderPipeline {
        self.device
            .create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("Render Pipeline"),
                layout: Some(&self.layout),
                vertex: wgpu::VertexState {
                    module: &self.vertex,
                    entry_point: &self.vertex_entry_name,
                    buffers: &[VertexBufferLayout {
                        array_stride: 4 * std::mem::size_of::<f32>() as wgpu::BufferAddress,
                        step_mode: wgpu::VertexStepMode::Vertex,
                        attributes: &[
                            wgpu::VertexAttribute {
                                format: wgpu::VertexFormat::Float32x2,
                                offset: 0,
                                shader_location: 0,
                            },
                            wgpu::VertexAttribute {
                                format: wgpu::VertexFormat::Float32x2,
                                offset: (2 * std::mem::size_of::<f32>()) as wgpu::BufferAddress,
                                shader_location: 1,
                            },
                        ],
                    }],
                },
                fragment: Some(wgpu::FragmentState {
                    module: &self.fragment,
                    entry_point: &self.fragment_entry_name,
                    targets: &[Some(wgpu::ColorTargetState {
                        format: framebuffer_format,
                        blend: Some(wgpu::BlendState::REPLACE),
                        write_mask: wgpu::ColorWrites::ALL,
                    })],
                }),
                primitive: wgpu::PrimitiveState {
                    topology: wgpu::PrimitiveTopology::TriangleStrip,
                    strip_index_format: None,
                    front_face: wgpu::FrontFace::Ccw,
                    cull_mode: None,
                    unclipped_depth: false,
                    polygon_mode: wgpu::PolygonMode::Fill,
                    conservative: false,
                },
                depth_stencil: None,
                multisample: wgpu::MultisampleState {
                    count: 1,
                    mask: !0,
                    alpha_to_coverage_enabled: false,
                },
                multiview: None,
            })
    }
}

impl WgpuGraphicsPipeline {
    pub fn new(
        device: Arc<wgpu::Device>,
        shader_assembly: &ShaderCompilerOutput<String, NagaWgslContext>,
        reflection: &ShaderReflection,
        render_pass_format: TextureFormat,
    ) -> Self {
        let layout = PipelineLayoutObjects::new(reflection, shader_assembly, device);
        let render_pipeline = layout.create_pipeline(render_pass_format);
        Self {
            layout,
            render_pipeline,
            format: render_pass_format,
        }
    }

    pub fn recompile(&mut self, format: TextureFormat) {
        let render_pipeline = self.layout.create_pipeline(format);
        self.render_pipeline = render_pipeline;
    }

    pub(crate) fn begin_rendering<'pass>(
        &'pass self,
        output: &RenderTarget<'pass, WgpuOutputView>,
        cmd: &'pass mut CommandEncoder,
    ) -> RenderPass<'pass> {
        let mut render_pass = cmd.begin_render_pass(&RenderPassDescriptor {
            label: Some("librashader"),
            color_attachments: &[Some(RenderPassColorAttachment {
                view: &output.output.view,
                resolve_target: None,
                ops: Operations {
                    load: wgpu::LoadOp::Clear(wgpu::Color {
                        r: 0.0,
                        g: 0.0,
                        b: 0.0,
                        a: 0.0,
                    }),
                    store: wgpu::StoreOp::Store,
                },
            })],
            depth_stencil_attachment: None,
            timestamp_writes: None,
            occlusion_query_set: None,
        });

        render_pass.set_scissor_rect(
            output.x as u32,
            output.y as u32,
            output.output.size.width,
            output.output.size.height,
        );

        render_pass.set_viewport(
            output.x,
            output.y,
            output.output.size.width as f32,
            output.output.size.height as f32,
            1.0,
            1.0,
        );

        render_pass.set_pipeline(&self.render_pipeline);
        render_pass
    }
}
