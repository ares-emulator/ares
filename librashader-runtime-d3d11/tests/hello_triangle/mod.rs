const WIDTH: i32 = 800;
const HEIGHT: i32 = 600;
const TITLE: &str = "librashader DirectX 11";

mod texture;

use windows::{
    core::*, Win32::Foundation::*, Win32::Graphics::Direct3D::Fxc::*, Win32::Graphics::Direct3D::*,
    Win32::Graphics::Direct3D11::*, Win32::Graphics::Dxgi::Common::*, Win32::Graphics::Dxgi::*,
    Win32::System::LibraryLoader::*, Win32::UI::WindowsAndMessaging::*,
};

static VERTEX_SHADER: &[u8] = b"
cbuffer cb : register(b0)
{
    row_major float4x4 projectionMatrix : packoffset(c0);
    row_major float4x4 modelMatrix : packoffset(c4);
    row_major float4x4 viewMatrix : packoffset(c8);
};

struct VertexInput
{
    float3 inPos : POSITION;
    float3 inColor : COLOR;
};

struct VertexOutput
{
    float3 color : COLOR;
    float4 position : SV_Position;
};

VertexOutput main(VertexInput vertexInput)
{
    float3 inColor = vertexInput.inColor;
    float3 inPos = vertexInput.inPos;
    float3 outColor = inColor;
    float4 position = mul(float4(inPos, 1.0), mul(modelMatrix, mul(viewMatrix, projectionMatrix)));

    VertexOutput output;
    output.position = position;
    output.color = outColor;
    return output;
}\0";

static PIXEL_SHADER: &[u8] = b"
struct PixelInput
{
    float3 color : COLOR;
};

struct PixelOutput
{
    float4 attachment0 : SV_Target0;
};

PixelOutput main(PixelInput pixelInput)
{
    float3 inColor = pixelInput.color;
    PixelOutput output;
    output.attachment0 = float4(inColor, 1.0f);
    return output;
}\0";

use gfx_maths::Mat4;
use std::mem::transmute;

pub trait DXSample {
    fn bind_to_window(&mut self, hwnd: &HWND) -> Result<()>;

    fn update(&mut self) {}
    fn render(&mut self) -> Result<()> {
        Ok(())
    }
    fn on_key_up(&mut self, _key: u8) {}
    fn on_key_down(&mut self, _key: u8) {}

    fn title(&self) -> String {
        TITLE.into()
    }

    fn window_size(&self) -> (i32, i32) {
        (WIDTH, HEIGHT)
    }
    fn resize(&mut self, w: u32, h: u32) -> Result<()>;
}

#[inline]
pub fn loword(l: usize) -> u32 {
    (l & 0xffff) as u32
}
#[inline]
pub fn hiword(l: usize) -> u32 {
    ((l >> 16) & 0xffff) as u32
}

fn run_sample<S>(mut sample: S) -> Result<()>
where
    S: DXSample,
{
    let instance = unsafe { GetModuleHandleA(None)? };

    let wc = WNDCLASSEXA {
        cbSize: std::mem::size_of::<WNDCLASSEXA>() as u32,
        style: CS_HREDRAW | CS_VREDRAW,
        lpfnWndProc: Some(wndproc::<S>),
        hInstance: HINSTANCE::from(instance),
        hCursor: unsafe { LoadCursorW(None, IDC_ARROW)? },
        lpszClassName: s!("RustWindowClass"),
        ..Default::default()
    };

    let size = sample.window_size();

    let atom = unsafe { RegisterClassExA(&wc) };
    debug_assert_ne!(atom, 0);

    let mut window_rect = RECT {
        left: 0,
        top: 0,
        right: size.0,
        bottom: size.1,
    };
    unsafe { AdjustWindowRect(&mut window_rect, WS_OVERLAPPEDWINDOW, false)? };

    let mut title = sample.title();

    title.push('\0');

    let hwnd = unsafe {
        CreateWindowExA(
            WINDOW_EX_STYLE::default(),
            s!("RustWindowClass"),
            PCSTR(title.as_ptr()),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            window_rect.right - window_rect.left,
            window_rect.bottom - window_rect.top,
            None, // no parent window
            None, // no menus
            instance,
            Some(&sample as *const _ as _),
        )
    };

    let hwnd = hwnd?;
    sample.bind_to_window(&hwnd).unwrap();
    unsafe { ShowWindow(hwnd, SW_SHOW) };

    loop {
        let mut message = MSG::default();

        if unsafe { PeekMessageA(&mut message, None, 0, 0, PM_REMOVE) }.into() {
            unsafe {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }

            if message.message == WM_QUIT {
                break;
            }
        }
    }

    Ok(())
}

fn sample_wndproc<S: DXSample>(sample: &mut S, message: u32, wparam: WPARAM) -> bool {
    match message {
        WM_KEYDOWN => {
            sample.on_key_down(wparam.0 as u8);
            true
        }
        WM_KEYUP => {
            sample.on_key_up(wparam.0 as u8);
            true
        }
        WM_PAINT => {
            sample.update();
            sample.render().unwrap();
            true
        }
        WM_SIZE => {
            sample.resize(loword(wparam.0), hiword(wparam.0)).unwrap();
            true
        }
        _ => false,
    }
}

extern "system" fn wndproc<S: DXSample>(
    window: HWND,
    message: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    match message {
        WM_CREATE => {
            unsafe {
                let create_struct: &CREATESTRUCTA = transmute(lparam);
                SetWindowLongPtrA(window, GWLP_USERDATA, create_struct.lpCreateParams as _);
            }
            LRESULT::default()
        }
        WM_DESTROY => {
            unsafe { PostQuitMessage(0) };
            LRESULT::default()
        }

        _ => {
            let user_data = unsafe { GetWindowLongPtrA(window, GWLP_USERDATA) };
            let sample = std::ptr::NonNull::<S>::new(user_data as _);
            let handled = sample.map_or(false, |mut s| {
                sample_wndproc(unsafe { s.as_mut() }, message, wparam)
            });

            if handled {
                LRESULT::default()
            } else {
                unsafe { DefWindowProcA(window, message, wparam, lparam) }
            }
        }
    }
}

#[repr(C)]
struct Vertex {
    position: [f32; 3],
    color: [f32; 3],
}

#[repr(C)]
#[derive(Default)]
struct TriangleUniforms {
    projection_matrix: Mat4,
    model_matrix: Mat4,
    view_matrix: Mat4,
}

pub mod d3d11_hello_triangle {

    use super::*;
    use std::path::Path;

    use librashader_common::{FilterMode, GetSize, ImageFormat, Size, Viewport, WrapMode};
    use librashader_runtime::image::Image;
    use librashader_runtime_d3d11::options::FilterChainOptionsD3D11;
    use librashader_runtime_d3d11::FilterChainD3D11;
    use std::slice;
    use std::time::Instant;
    use texture::ExampleTexture;

    pub struct Sample {
        pub dxgi_factory: IDXGIFactory4,
        pub device: ID3D11Device,
        pub context: ID3D11DeviceContext,
        pub resources: Option<Resources>,
        pub filter: FilterChainD3D11,
        pub lut: Option<ID3D11Texture2D>,
    }

    pub struct Resources {
        pub swapchain: IDXGISwapChain,
        pub depth_buffer: ID3D11Texture2D,
        pub depth_stencil_view: ID3D11DepthStencilView,
        pub triangle_vertices: ID3D11Buffer,
        pub triangle_indices: ID3D11Buffer,
        pub triangle_uniforms: ID3D11Buffer,
        pub vs: ID3D11VertexShader,
        pub ps: ID3D11PixelShader,
        pub input_layout: ID3D11InputLayout,
        pub frame_start: Instant,
        pub frame_end: Instant,
        pub elapsed: f32,
        triangle_uniform_values: TriangleUniforms,
        pub renderbuffer: ID3D11Texture2D,
        pub renderbufffer_rtv: ID3D11RenderTargetView,
        pub backbuffer: Option<ID3D11Texture2D>,
        pub backbuffer_rtv: Option<ID3D11RenderTargetView>,
        pub viewport: D3D11_VIEWPORT,
        pub shader_output: Option<ID3D11Texture2D>,
        pub frame_count: usize,
        pub deferred_context: ID3D11DeviceContext,
    }

    impl Sample {
        pub(crate) fn new(
            filter: impl AsRef<Path>,
            filter_options: Option<&FilterChainOptionsD3D11>,
            image: Option<Image>,
        ) -> Result<Self> {
            let (dxgi_factory, device, context) = create_device()?;
            let filter = unsafe {
                FilterChainD3D11::load_from_path(filter, &device, filter_options).unwrap()
            };
            let lut = if let Some(image) = image {
                let lut = ExampleTexture::new(
                    &device,
                    &context,
                    &image,
                    D3D11_TEXTURE2D_DESC {
                        Width: image.size.width,
                        Height: image.size.height,
                        MipLevels: 1,
                        ArraySize: 0,
                        Format: ImageFormat::R8G8B8A8Unorm.into(),
                        SampleDesc: DXGI_SAMPLE_DESC {
                            Count: 1,
                            Quality: 0,
                        },
                        Usage: D3D11_USAGE_DYNAMIC,
                        BindFlags: D3D11_BIND_SHADER_RESOURCE.0 as u32,
                        CPUAccessFlags: Default::default(),
                        MiscFlags: Default::default(),
                    },
                    FilterMode::Linear,
                    WrapMode::ClampToEdge,
                )
                .unwrap();
                Some(lut.handle)
            } else {
                None
            };
            Ok(Sample {
                filter,
                dxgi_factory,
                device,
                context,
                resources: None,
                lut,
            })
        }
    }
    impl DXSample for Sample {
        fn bind_to_window(&mut self, hwnd: &HWND) -> Result<()> {
            let swapchain = create_swapchain(&self.dxgi_factory, &self.device, *hwnd)?;
            let (rtv, backbuffer) = create_rtv(&self.device, &swapchain)?;
            let (depth_buffer, depth_stencil_view) = create_depth_buffer(&self.device)?;
            let (triangle_vbo, triangle_indices) = create_triangle_buffers(&self.device)?;
            let triangle_uniforms = create_triangle_uniforms(&self.device).unwrap();

            let vs_blob = compile_shader(VERTEX_SHADER, b"main\0", b"vs_5_0\0")?;
            let ps_blob = compile_shader(PIXEL_SHADER, b"main\0", b"ps_5_0\0")?;

            let vs_compiled = unsafe {
                // SAFETY: slice as valid for as long as vs_blob is alive.
                slice::from_raw_parts(
                    vs_blob.GetBufferPointer().cast::<u8>(),
                    vs_blob.GetBufferSize(),
                )
            };

            let vs = unsafe {
                let mut vso = None;
                self.device
                    .CreateVertexShader(vs_compiled, None, Some(&mut vso))?;
                vso.unwrap()
            };

            let ps = unsafe {
                let ps = slice::from_raw_parts(
                    ps_blob.GetBufferPointer().cast::<u8>(),
                    ps_blob.GetBufferSize(),
                );
                let mut pso = None;
                self.device.CreatePixelShader(ps, None, Some(&mut pso))?;
                pso.unwrap()
            };

            let (input_layout, stencil_state, raster_state) =
                create_pipeline_state(&self.device, vs_compiled)?;

            unsafe {
                self.context.OMSetDepthStencilState(&stencil_state, 1);
                self.context.RSSetState(&raster_state);
            }

            let (renderbuffer, render_rtv) = unsafe {
                let mut renderbuffer = None;
                let mut rtv = None;
                let mut desc = Default::default();

                backbuffer.GetDesc(&mut desc);
                desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE.0 as u32;

                self.device
                    .CreateTexture2D(&desc, None, Some(&mut renderbuffer))?;
                let renderbuffer = renderbuffer.unwrap();
                self.device
                    .CreateRenderTargetView(&renderbuffer, None, Some(&mut rtv))?;
                (renderbuffer, rtv.unwrap())
            };

            let mut context = None;
            unsafe { self.device.CreateDeferredContext(0, Some(&mut context))? };
            let context = context.expect("Could not create deferred context");

            self.resources = Some(Resources {
                swapchain,
                backbuffer_rtv: Some(rtv),
                backbuffer: Some(backbuffer),
                depth_buffer,
                depth_stencil_view,
                triangle_vertices: triangle_vbo,
                triangle_indices,
                triangle_uniforms,
                vs,
                ps,
                input_layout,
                frame_end: Instant::now(),
                frame_start: Instant::now(),
                elapsed: 0f32,
                triangle_uniform_values: Default::default(),
                renderbuffer,
                renderbufffer_rtv: render_rtv,
                deferred_context: context,
                viewport: D3D11_VIEWPORT {
                    TopLeftX: 0.0,
                    TopLeftY: 0.0,
                    Width: WIDTH as f32,
                    Height: HEIGHT as f32,
                    MinDepth: D3D11_MIN_DEPTH,
                    MaxDepth: D3D11_MAX_DEPTH,
                },
                shader_output: None,
                frame_count: 0usize,
            });

            Ok(())
        }

        fn resize(&mut self, _w: u32, _h: u32) -> Result<()> {
            unsafe {
                if let Some(resources) = self.resources.as_mut() {
                    drop(resources.backbuffer_rtv.take());
                    drop(resources.backbuffer.take());
                    resources
                        .swapchain
                        .ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG(0))
                        .unwrap_or_else(|f| eprintln!("{f:?}"));
                    let (rtv, backbuffer) = create_rtv(&self.device, &resources.swapchain)?;

                    resources.backbuffer = Some(backbuffer);
                    resources.backbuffer_rtv = Some(rtv);
                }
            }
            Ok(())
        }
        fn render(&mut self) -> Result<()> {
            let Some(resources) = &mut self.resources else {
                return Ok(());
            };

            resources.frame_end = Instant::now();
            let time = resources.frame_end - resources.frame_start;
            let time = time.as_secs() as f32 * 1000.0;

            // framelimit set to 60fps
            if time < (1000.0f32 / 60.0f32) {
                return Ok(());
            }

            resources.elapsed += 0.0000001 * time;
            resources.elapsed %= 6.283_185_5_f32;

            // resources.triangle_uniform_values.model_matrix = Mat4::rotate(Quaternion::axis_angle(Vec3::new(0.0, 0.0, 1.0), resources.elapsed));
            resources.triangle_uniform_values.model_matrix = Mat4::identity();

            let buffer_number = 0;

            unsafe {
                let mut mapped_resource = Default::default();
                self.context.Map(
                    &resources.triangle_uniforms,
                    0,
                    D3D11_MAP_WRITE_DISCARD,
                    0,
                    Some(&mut mapped_resource),
                )?;
                std::ptr::copy_nonoverlapping(
                    &resources.triangle_uniform_values,
                    mapped_resource.pData.cast(),
                    1,
                );
                self.context.Unmap(&resources.triangle_uniforms, 0);
            }

            unsafe {
                self.context.VSSetConstantBuffers(
                    buffer_number,
                    Some(&[Some(resources.triangle_uniforms.clone())]),
                );
                self.context.OMSetRenderTargets(
                    Some(&[Some(resources.renderbufffer_rtv.clone())]),
                    &resources.depth_stencil_view,
                );
                self.context.RSSetViewports(Some(&[resources.viewport]))
            }

            unsafe {
                let color = [0.3, 0.4, 0.6, 1.0];
                self.context
                    .ClearRenderTargetView(&resources.renderbufffer_rtv, &color);
                self.context.ClearDepthStencilView(
                    &resources.depth_stencil_view,
                    D3D11_CLEAR_DEPTH.0,
                    1.0,
                    0,
                );
                self.context.IASetInputLayout(&resources.input_layout);
            }

            unsafe {
                self.context.VSSetShader(&resources.vs, None);
                self.context.PSSetShader(&resources.ps, None);

                let stride = std::mem::size_of::<Vertex>() as u32;
                let offset = 0;
                self.context.IASetVertexBuffers(
                    0,
                    1,
                    Some(&Some(resources.triangle_vertices.clone())),
                    Some(&stride),
                    Some(&offset),
                );
                self.context
                    .IASetIndexBuffer(&resources.triangle_indices, DXGI_FORMAT_R32_UINT, 0);
                self.context
                    .IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            }

            unsafe {
                self.context.DrawIndexed(3, 0, 0);
                self.context.OMSetRenderTargets(None, None);
            }

            unsafe {
                let input = if let Some(lut) = &self.lut {
                    lut.clone()
                } else {
                    resources.renderbuffer.clone()
                };

                let mut tex2d_desc = Default::default();
                input.GetDesc(&mut tex2d_desc);
                let mut backbuffer_desc = Default::default();
                resources
                    .backbuffer
                    .as_ref()
                    .unwrap()
                    .GetDesc(&mut backbuffer_desc);

                let mut input_srv = None;
                self.device.CreateShaderResourceView(
                    &input,
                    Some(&D3D11_SHADER_RESOURCE_VIEW_DESC {
                        Format: tex2d_desc.Format,
                        ViewDimension: D3D_SRV_DIMENSION_TEXTURE2D,
                        Anonymous: D3D11_SHADER_RESOURCE_VIEW_DESC_0 {
                            Texture2D: D3D11_TEX2D_SRV {
                                MostDetailedMip: 0,
                                MipLevels: u32::MAX,
                            },
                        },
                    }),
                    Some(&mut input_srv),
                )?;
                let srv = input_srv.unwrap();

                // eprintln!("w: {} h: {}", backbuffer_desc.Width, backbuffer_desc.Height);
                let output = resources.backbuffer_rtv.as_ref().unwrap();
                let size: Size<u32> = output.size().unwrap();
                self.filter
                    .frame(
                        None,
                        &srv,
                        &Viewport {
                            x: 100f32,
                            y: 100f32,
                            output,
                            mvp: None,
                            size: Size {
                                width: size.width - 200,
                                height: size.height - 200,
                            },
                        },
                        resources.frame_count,
                        None,
                    )
                    .unwrap();

                // let mut command_list = None;
                // resources
                //     .deferred_context
                //     .FinishCommandList(false, Some(&mut command_list))?;
                // let command_list = command_list.unwrap();
                // self.context.ExecuteCommandList(&command_list, false);
                // self.context.CopyResource(&resources.backbuffer, &backup);
            }

            unsafe {
                resources
                    .swapchain
                    .Present(0, DXGI_PRESENT::default())
                    .ok()?;
            }
            resources.frame_count += 1;
            Ok(())
        }
    }

    fn create_rtv(
        device: &ID3D11Device,
        swapchain: &IDXGISwapChain,
    ) -> Result<(ID3D11RenderTargetView, ID3D11Texture2D)> {
        unsafe {
            let backbuffer: ID3D11Texture2D = swapchain.GetBuffer(0)?;
            let mut rtv = None;
            device.CreateRenderTargetView(&backbuffer, None, Some(&mut rtv))?;

            Ok((rtv.unwrap(), backbuffer))
        }
    }
    fn create_pipeline_state(
        device: &ID3D11Device,
        vs_blob: &[u8],
    ) -> Result<(
        ID3D11InputLayout,
        ID3D11DepthStencilState,
        ID3D11RasterizerState,
    )> {
        unsafe {
            let mut input_layout = None;
            device.CreateInputLayout(
                &[
                    D3D11_INPUT_ELEMENT_DESC {
                        SemanticName: s!("POSITION"),
                        SemanticIndex: 0,
                        Format: DXGI_FORMAT_R32G32B32_FLOAT,
                        InputSlot: 0,
                        AlignedByteOffset: 0,
                        InputSlotClass: D3D11_INPUT_PER_VERTEX_DATA,
                        InstanceDataStepRate: 0,
                    },
                    D3D11_INPUT_ELEMENT_DESC {
                        SemanticName: s!("COLOR"),
                        SemanticIndex: 0,
                        Format: DXGI_FORMAT_R32G32B32_FLOAT,
                        InputSlot: 0,
                        AlignedByteOffset: D3D11_APPEND_ALIGNED_ELEMENT,
                        InputSlotClass: D3D11_INPUT_PER_VERTEX_DATA,
                        InstanceDataStepRate: 0,
                    },
                ],
                vs_blob,
                Some(&mut input_layout),
            )?;

            let input_layout = input_layout.unwrap();

            let mut stencil_state = None;
            device.CreateDepthStencilState(
                &D3D11_DEPTH_STENCIL_DESC {
                    DepthEnable: BOOL::from(true),
                    DepthWriteMask: D3D11_DEPTH_WRITE_MASK_ALL,
                    DepthFunc: D3D11_COMPARISON_LESS,
                    StencilEnable: BOOL::from(true),
                    StencilReadMask: 0xff,
                    StencilWriteMask: 0xff,
                    FrontFace: D3D11_DEPTH_STENCILOP_DESC {
                        StencilFailOp: D3D11_STENCIL_OP_KEEP,
                        StencilDepthFailOp: D3D11_STENCIL_OP_INCR,
                        StencilPassOp: D3D11_STENCIL_OP_KEEP,
                        StencilFunc: D3D11_COMPARISON_ALWAYS,
                    },
                    BackFace: D3D11_DEPTH_STENCILOP_DESC {
                        StencilFailOp: D3D11_STENCIL_OP_KEEP,
                        StencilDepthFailOp: D3D11_STENCIL_OP_DECR,
                        StencilPassOp: D3D11_STENCIL_OP_KEEP,
                        StencilFunc: D3D11_COMPARISON_ALWAYS,
                    },
                },
                Some(&mut stencil_state),
            )?;
            let stencil_state = stencil_state.unwrap();

            let mut rasterizer_state = None;
            device.CreateRasterizerState(
                &D3D11_RASTERIZER_DESC {
                    AntialiasedLineEnable: BOOL::from(false),
                    CullMode: D3D11_CULL_NONE,
                    DepthBias: 0,
                    DepthBiasClamp: 0.0f32,
                    DepthClipEnable: BOOL::from(true),
                    FillMode: D3D11_FILL_SOLID,
                    FrontCounterClockwise: BOOL::from(false),
                    MultisampleEnable: BOOL::from(false),
                    ScissorEnable: BOOL::from(false),
                    SlopeScaledDepthBias: 0.0f32,
                },
                Some(&mut rasterizer_state),
            )?;

            let rasterizer_state = rasterizer_state.unwrap();
            Ok((input_layout, stencil_state, rasterizer_state))
        }
    }

    fn create_depth_buffer(
        device: &ID3D11Device,
    ) -> Result<(ID3D11Texture2D, ID3D11DepthStencilView)> {
        unsafe {
            let mut buffer = None;
            device.CreateTexture2D(
                &D3D11_TEXTURE2D_DESC {
                    Width: WIDTH as u32,
                    Height: HEIGHT as u32,
                    MipLevels: 1,
                    ArraySize: 1,
                    Format: DXGI_FORMAT_D24_UNORM_S8_UINT,
                    SampleDesc: DXGI_SAMPLE_DESC {
                        Count: 1,
                        Quality: 0,
                    },
                    Usage: D3D11_USAGE_DEFAULT,
                    BindFlags: D3D11_BIND_DEPTH_STENCIL.0 as u32,
                    CPUAccessFlags: Default::default(),
                    MiscFlags: Default::default(),
                },
                None,
                Some(&mut buffer),
            )?;

            let buffer = buffer.unwrap();

            let mut view = None;
            device.CreateDepthStencilView(
                &buffer,
                Some(&D3D11_DEPTH_STENCIL_VIEW_DESC {
                    Format: DXGI_FORMAT_D24_UNORM_S8_UINT,
                    ViewDimension: D3D11_DSV_DIMENSION_TEXTURE2D,
                    Anonymous: D3D11_DEPTH_STENCIL_VIEW_DESC_0 {
                        Texture2D: D3D11_TEX2D_DSV { MipSlice: 0 },
                    },
                    ..Default::default()
                }),
                Some(&mut view),
            )?;

            let view = view.unwrap();

            Ok((buffer, view))
        }
    }

    fn create_triangle_uniforms(device: &ID3D11Device) -> Result<ID3D11Buffer> {
        let mut buffer = None;
        unsafe {
            device.CreateBuffer(
                &D3D11_BUFFER_DESC {
                    ByteWidth: (std::mem::size_of::<TriangleUniforms>()) as u32,
                    Usage: D3D11_USAGE_DYNAMIC,
                    BindFlags: D3D11_BIND_CONSTANT_BUFFER.0 as u32,
                    CPUAccessFlags: D3D11_CPU_ACCESS_WRITE.0 as u32,
                    MiscFlags: Default::default(),
                    StructureByteStride: 0,
                },
                None,
                Some(&mut buffer),
            )?;
        }
        Ok(buffer.unwrap())
    }

    fn create_triangle_buffers(device: &ID3D11Device) -> Result<(ID3D11Buffer, ID3D11Buffer)> {
        let vertices = [
            Vertex {
                position: [0.5f32, -0.5, 0.0],
                color: [1.0, 0.0, 0.0],
            },
            Vertex {
                position: [-0.5, -0.5, 0.0],
                color: [0.0, 1.0, 0.0],
            },
            Vertex {
                position: [0.0, 0.5, 0.0],
                color: [0.0, 0.0, 1.0],
            },
        ];

        let indices = [0, 1, 2];
        unsafe {
            let mut vertex_buffer = None;
            device.CreateBuffer(
                &D3D11_BUFFER_DESC {
                    ByteWidth: (std::mem::size_of::<Vertex>() * vertices.len()) as u32,
                    Usage: D3D11_USAGE_DEFAULT,
                    BindFlags: D3D11_BIND_VERTEX_BUFFER.0 as u32,
                    CPUAccessFlags: Default::default(),
                    MiscFlags: Default::default(),
                    StructureByteStride: 0,
                },
                Some(&D3D11_SUBRESOURCE_DATA {
                    pSysMem: vertices.as_ptr().cast(),
                    SysMemPitch: 0,
                    SysMemSlicePitch: 0,
                }),
                Some(&mut vertex_buffer),
            )?;

            let mut index_buffer = None;
            device.CreateBuffer(
                &D3D11_BUFFER_DESC {
                    ByteWidth: (std::mem::size_of::<u32>() * indices.len()) as u32,
                    Usage: D3D11_USAGE_DEFAULT,
                    BindFlags: D3D11_BIND_INDEX_BUFFER.0 as u32,
                    CPUAccessFlags: Default::default(),
                    MiscFlags: Default::default(),
                    StructureByteStride: 0,
                },
                Some(&D3D11_SUBRESOURCE_DATA {
                    pSysMem: indices.as_ptr().cast(),
                    SysMemPitch: 0,
                    SysMemSlicePitch: 0,
                }),
                Some(&mut index_buffer),
            )?;

            Ok((vertex_buffer.unwrap(), index_buffer.unwrap()))
        }
    }
    fn create_device() -> Result<(IDXGIFactory4, ID3D11Device, ID3D11DeviceContext)> {
        let dxgi_factory: IDXGIFactory4 = unsafe { CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG) }?;
        let feature_levels = vec![D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1];

        let mut out_device = None;
        let mut out_context = None;
        let mut _out_feature_level = D3D_FEATURE_LEVEL_11_0;

        unsafe {
            D3D11CreateDevice(
                None,
                D3D_DRIVER_TYPE_HARDWARE,
                HMODULE::default(),
                D3D11_CREATE_DEVICE_DEBUG,
                Some(&feature_levels),
                D3D11_SDK_VERSION,
                Some(&mut out_device),
                Some(&mut _out_feature_level),
                Some(&mut out_context),
            )
        }?;
        Ok((dxgi_factory, out_device.unwrap(), out_context.unwrap()))
    }

    fn create_swapchain(
        fac: &IDXGIFactory4,
        device: &ID3D11Device,
        hwnd: HWND,
    ) -> Result<IDXGISwapChain> {
        let swapchain_desc = DXGI_SWAP_CHAIN_DESC {
            BufferDesc: DXGI_MODE_DESC {
                Width: WIDTH as u32,
                Height: HEIGHT as u32,
                RefreshRate: DXGI_RATIONAL {
                    Numerator: 0,
                    Denominator: 1,
                },
                Format: DXGI_FORMAT_R8G8B8A8_UNORM,
                ScanlineOrdering: DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                Scaling: DXGI_MODE_SCALING_UNSPECIFIED,
            },
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                Quality: 0,
            },
            BufferUsage: DXGI_USAGE_RENDER_TARGET_OUTPUT,
            BufferCount: 1,
            OutputWindow: hwnd,
            Windowed: BOOL(1),
            SwapEffect: DXGI_SWAP_EFFECT_DISCARD,
            Flags: 0,
        };

        let mut swap_chain = None;
        unsafe {
            fac.CreateSwapChain(device, &swapchain_desc, &mut swap_chain)
                .ok()?;
        }

        Ok(swap_chain.expect("[dx11] swapchain creation failed."))
    }

    fn compile_shader(source: &[u8], entry: &[u8], version: &[u8]) -> Result<ID3DBlob> {
        unsafe {
            let mut blob = None;
            D3DCompile(
                source.as_ptr().cast(),
                source.len(),
                None,
                None,
                None,
                PCSTR(entry.as_ptr()),
                PCSTR(version.as_ptr()),
                D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                0,
                &mut blob,
                None,
            )?;

            Ok(blob.unwrap())
        }
    }
}

pub fn main<S: DXSample>(sample: S) -> Result<()> {
    run_sample(sample)?;

    Ok(())
}
