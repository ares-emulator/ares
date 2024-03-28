use std::sync::Arc;
use winit::{
    event::*,
    window::{Window, WindowBuilder},
};

use librashader_common::Viewport;
use librashader_presets::ShaderPreset;
use librashader_runtime_wgpu::FilterChainWgpu;
use wgpu::util::DeviceExt;
use winit::event_loop::EventLoopBuilder;
use winit::platform::windows::EventLoopBuilderExtWindows;

#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;

#[repr(C)]
#[derive(Clone, Copy, Debug, bytemuck::Pod, bytemuck::Zeroable)]
struct Vertex {
    position: [f32; 3],
    color: [f32; 3],
}
impl Vertex {
    fn desc<'a>() -> wgpu::VertexBufferLayout<'a> {
        wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<Vertex>() as wgpu::BufferAddress,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &[
                wgpu::VertexAttribute {
                    offset: 0,
                    shader_location: 0,
                    format: wgpu::VertexFormat::Float32x3,
                },
                wgpu::VertexAttribute {
                    offset: std::mem::size_of::<[f32; 3]>() as wgpu::BufferAddress,
                    shader_location: 1,
                    format: wgpu::VertexFormat::Float32x3,
                },
            ],
        }
    }
}

const VERTICES: &[Vertex] = &[
    Vertex {
        // top
        position: [0.0, 0.5, 0.0],
        color: [1.0, 0.0, 0.0],
    },
    Vertex {
        // bottom left
        position: [-0.5, -0.5, 0.0],
        color: [0.0, 1.0, 0.0],
    },
    Vertex {
        // bottom right
        position: [0.5, -0.5, 0.0],
        color: [0.0, 0.0, 1.0],
    },
];

struct State<'a> {
    surface: wgpu::Surface<'a>,
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    config: wgpu::SurfaceConfiguration,
    size: winit::dpi::PhysicalSize<u32>,
    clear_color: wgpu::Color,

    render_pipeline: wgpu::RenderPipeline,

    vertex_buffer: wgpu::Buffer,
    num_vertices: u32,
    chain: FilterChainWgpu,
    frame_count: usize,
}
impl<'a> State<'a> {
    async fn new(window: &'a Window) -> Self {
        let size = window.inner_size();

        let instance = wgpu::Instance::default();
        let surface = instance.create_surface(window).unwrap();
        // NOTE: could be none, see: https://sotrh.github.io/learn-wgpu/beginner/tutorial2-surface/#state-new
        let adapter = instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::default(),
                compatible_surface: Some(&surface),
                force_fallback_adapter: false,
            })
            .await
            .unwrap();

        let (device, queue) = adapter
            .request_device(
                &wgpu::DeviceDescriptor {
                    required_features: wgpu::Features::ADDRESS_MODE_CLAMP_TO_BORDER,
                    required_limits: wgpu::Limits::default(),
                    label: None,
                },
                None,
            )
            .await
            .unwrap();
        let swapchain_capabilities = surface.get_capabilities(&adapter);
        let swapchain_format = swapchain_capabilities.formats[0];

        let config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_DST,
            format: swapchain_format,
            width: size.width,
            height: size.height,
            present_mode: wgpu::PresentMode::Fifo,
            desired_maximum_frame_latency: 2,
            alpha_mode: swapchain_capabilities.alpha_modes[0],
            view_formats: vec![],
        };

        let device = Arc::new(device);
        let queue = Arc::new(queue);
        //
        // let preset = ShaderPreset::try_parse(
        //     "../test/basic.slangp",
        // )
        // .unwrap();
        //
        let preset = ShaderPreset::try_parse("../test/shaders_slang/test/history.slangp").unwrap();

        // let preset = ShaderPreset::try_parse(
        //     "../test/shaders_slang/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp",
        // )
        // .unwrap();

        let chain = FilterChainWgpu::load_from_preset(
            preset,
            Arc::clone(&device),
            Arc::clone(&queue),
            None,
        )
        .unwrap();

        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Shader"),
            source: wgpu::ShaderSource::Wgsl(include_str!("../shader/triangle.wgsl").into()),
        });
        let render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Render Pipeline Layout"),
                bind_group_layouts: &[],
                push_constant_ranges: &[],
            });
        let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Render Pipeline"),
            layout: Some(&render_pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: "vs_main",
                buffers: &[Vertex::desc()],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: "fs_main",
                targets: &[Some(wgpu::ColorTargetState {
                    format: config.format,
                    blend: Some(wgpu::BlendState::REPLACE),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: Some(wgpu::Face::Back),
                polygon_mode: wgpu::PolygonMode::Fill,
                unclipped_depth: false,
                conservative: false,
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });

        let vertex_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("triangle vertices"),
            contents: bytemuck::cast_slice(VERTICES),
            usage: wgpu::BufferUsages::VERTEX,
        });

        let clear_color = wgpu::Color {
            r: 0.1,
            g: 0.2,
            b: 0.3,
            a: 1.0,
        };
        let num_vertices = VERTICES.len() as u32;
        Self {
            surface,
            device,
            queue,
            config,
            size,
            clear_color,
            render_pipeline,
            vertex_buffer,
            num_vertices,
            chain,
            frame_count: 0,
        }
    }
    fn resize(&mut self, new_size: winit::dpi::PhysicalSize<u32>) {
        if new_size.width > 0 && new_size.height > 0 {
            self.size = new_size;
            self.config.width = new_size.width;
            self.config.height = new_size.height;
            self.surface.configure(&self.device, &self.config);
        }
    }
    fn input(&mut self, _event: &WindowEvent) -> bool {
        false
    }
    fn update(&mut self) {}
    fn render(&mut self) -> Result<(), wgpu::SurfaceError> {
        let output = self.surface.get_current_texture()?;

        let render_output = Arc::new(self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some("rendertexture"),
            size: output.texture.size(),
            mip_level_count: output.texture.mip_level_count(),
            sample_count: output.texture.sample_count(),
            dimension: output.texture.dimension(),
            format: output.texture.format(),
            usage: wgpu::TextureUsages::TEXTURE_BINDING
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::COPY_DST
                | wgpu::TextureUsages::COPY_SRC,
            view_formats: &[output.texture.format()],
        }));

        let filter_output = Arc::new(self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some("filteroutput"),
            size: output.texture.size(),
            mip_level_count: output.texture.mip_level_count(),
            sample_count: output.texture.sample_count(),
            dimension: output.texture.dimension(),
            format: output.texture.format(),
            usage: wgpu::TextureUsages::TEXTURE_BINDING
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::COPY_DST
                | wgpu::TextureUsages::COPY_SRC,
            view_formats: &[output.texture.format()],
        }));

        let view = render_output.create_view(&wgpu::TextureViewDescriptor::default());

        let filter_view = filter_output.create_view(&wgpu::TextureViewDescriptor::default());

        let mut encoder = self
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Render Encoder"),
            });

        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Render Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(self.clear_color),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });
            render_pass.set_pipeline(&self.render_pipeline);
            render_pass.set_vertex_buffer(0, self.vertex_buffer.slice(..));
            render_pass.draw(0..self.num_vertices, 0..1);
        }

        self.chain
            .frame(
                Arc::clone(&render_output),
                &Viewport {
                    x: 0.0,
                    y: 0.0,
                    mvp: None,
                    output: librashader_runtime_wgpu::WgpuOutputView::new_from_raw(
                        &filter_view,
                        filter_output.size().into(),
                        filter_output.format(),
                    ),
                },
                &mut encoder,
                self.frame_count,
                None,
            )
            .expect("failed to draw frame");

        encoder.copy_texture_to_texture(
            filter_output.as_image_copy(),
            output.texture.as_image_copy(),
            output.texture.size(),
        );

        self.queue.submit(std::iter::once(encoder.finish()));
        output.present();

        self.frame_count += 1;
        Ok(())
    }
}

pub fn run() {
    env_logger::init();

    let event_loop = EventLoopBuilder::new()
        .with_any_thread(true)
        .with_dpi_aware(true)
        .build()
        .unwrap();
    let window = WindowBuilder::new().build(&event_loop).unwrap();

    pollster::block_on(async {
        let mut state = State::new(&window).await;
        event_loop
            .run(|event, target| {
                match event {
                    Event::WindowEvent {
                        window_id: _,
                        event,
                    } => match event {
                        WindowEvent::Resized(new_size) => {
                            state.resize(new_size);
                            // On macos the window needs to be redrawn manually after resizing
                            window.request_redraw();
                        }
                        WindowEvent::RedrawRequested => {
                            state.update();
                            match state.render() {
                                Ok(_) => {}
                                Err(wgpu::SurfaceError::Lost | wgpu::SurfaceError::Outdated) => {
                                    state.resize(state.size)
                                }
                                Err(wgpu::SurfaceError::OutOfMemory) => target.exit(),
                                Err(wgpu::SurfaceError::Timeout) => log::warn!("Surface timeout"),
                            }
                        }
                        WindowEvent::CloseRequested => target.exit(),
                        _ => {}
                    },
                    Event::AboutToWait => window.request_redraw(),
                    _ => {}
                }
            })
            .unwrap();
    });
}
