const WIDTH: i32 = 800;
const HEIGHT: i32 = 600;
const TITLE: &str = "librashader DirectX 9";

use windows::{
    core::*, Win32::Foundation::*, Win32::Graphics::Direct3D9::*, Win32::System::LibraryLoader::*,
    Win32::UI::WindowsAndMessaging::*,
};

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

// #[repr(C)]
// struct Vertex {
//     position: [f32; 3],
//     color: [f32; 3],
// }

#[repr(C)]
struct Vertex {
    position: [f32; 4],
    color: u32,
}

#[repr(C)]
#[derive(Default)]
struct TriangleUniforms {
    projection_matrix: Mat4,
    model_matrix: Mat4,
    view_matrix: Mat4,
}

pub mod d3d9_hello_triangle {
    use super::*;

    use std::path::{Path, PathBuf};

    use librashader_common::{GetSize, Viewport};
    use librashader_runtime::image::{Image, UVDirection, ARGB8, BGRA8, RGBA8};
    use librashader_runtime_d3d9::options::FilterChainOptionsD3D9;
    use librashader_runtime_d3d9::FilterChainD3D9;
    use std::time::Instant;

    pub struct Sample {
        pub direct3d: IDirect3D9,
        pub resources: Option<Resources>,
        pub filter: PathBuf,
    }

    pub struct Resources {
        pub device: IDirect3DDevice9,
        pub filter: FilterChainD3D9,
        // pub depth_buffer: ID3D11Texture2D,
        // pub depth_stencil_view: ID3D11DepthStencilView,
        // pub triangle_vertices: ID3D11Buffer,
        // pub triangle_indices: ID3D11Buffer,
        // pub triangle_uniforms: ID3D11Buffer,
        // pub vs: ID3D11VertexShader,
        // pub ps: ID3D11PixelShader,
        // pub input_layout: ID3D11InputLayout,
        pub frame_start: Instant,
        pub frame_end: Instant,
        pub elapsed: f32,
        pub frame_count: usize,
        pub renderbuffer: IDirect3DTexture9,
        // pub renderbufffer_rtv: ID3D11RenderTargetView,
        // pub backbuffer: Option<ID3D11Texture2D>,
        // pub backbuffer_rtv: Option<ID3D11RenderTargetView>,
        // pub viewport: D3D11_VIEWPORT,
        // pub shader_output: Option<ID3D11Texture2D>,
        // pub deferred_context: ID3D11DeviceContext,
        pub vbo: IDirect3DVertexBuffer9,
        pub vao: IDirect3DVertexDeclaration9,
        pub texture: IDirect3DTexture9,
    }

    impl Sample {
        pub(crate) fn new(filter: impl AsRef<Path>) -> Result<Self> {
            // unsafe {
            //     let mut debug: Option<ID3D12Debug> = None;
            //     if let Some(debug) = D3D12GetDebugInterface(&mut debug).ok().and(debug) {
            //         eprintln!("enabling debug");
            //         debug.EnableDebugLayer();
            //     }
            // }
            // let direct3d = unsafe { Direct3DCreate9On12(D3D_SDK_VERSION, std::ptr::null_mut(), 0).unwrap() };

            let direct3d = unsafe { Direct3DCreate9(D3D_SDK_VERSION).unwrap() };

            Ok(Sample {
                filter: filter.as_ref().to_path_buf(),
                direct3d,
                resources: None,
            })
        }
    }
    impl DXSample for Sample {
        fn bind_to_window(&mut self, hwnd: &HWND) -> Result<()> {
            let device = create_device(&self.direct3d, *hwnd)?;
            let filter = unsafe {
                FilterChainD3D9::load_from_path(
                    &self.filter,
                    &device,
                    Some(&FilterChainOptionsD3D9 {
                        force_no_mipmaps: false,
                        disable_cache: true,
                    }),
                )
                .unwrap()
            };
            let (vbo, vao) = create_triangle_buffers(&device)?;

            let renderbuffer = unsafe {
                let mut tex = None;
                device.CreateTexture(
                    WIDTH as u32,
                    HEIGHT as u32,
                    1,
                    D3DUSAGE_RENDERTARGET as u32,
                    D3DFMT_A8R8G8B8,
                    D3DPOOL_DEFAULT,
                    &mut tex,
                    std::ptr::null_mut(),
                )?;

                tex.unwrap()
            };

            const IMAGE_PATH: &str = "../triangle.png";

            let image = Image::<BGRA8>::load(IMAGE_PATH, UVDirection::TopLeft)
                .expect("triangle.png not found");

            let texture = unsafe {
                let mut texture = None;
                device.CreateTexture(
                    image.size.width,
                    image.size.height,
                    1,
                    0,
                    D3DFMT_A8R8G8B8,
                    D3DPOOL_MANAGED,
                    &mut texture,
                    std::ptr::null_mut(),
                )?;

                texture.unwrap()
            };

            unsafe {
                let mut lock = D3DLOCKED_RECT::default();
                texture.LockRect(0, &mut lock, std::ptr::null_mut(), 0)?;
                std::ptr::copy_nonoverlapping(
                    image.bytes.as_ptr(),
                    lock.pBits.cast(),
                    image.bytes.len(),
                );
                texture.UnlockRect(0)?;
            }

            self.resources = Some(Resources {
                device,
                filter,
                vbo,
                vao,
                frame_end: Instant::now(),
                frame_start: Instant::now(),
                elapsed: 0f32,
                texture,
                frame_count: 0usize,
                renderbuffer,
            });

            Ok(())
        }

        // fn resize(&mut self, _w: u32, _h: u32) -> Result<()> {
        //     unsafe {
        //         if let Some(resources) = self.resources.as_mut() {
        //             drop(resources.backbuffer_rtv.take());
        //             drop(resources.backbuffer.take());
        //             resources
        //                 .swapchain
        //                 .ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)
        //                 .unwrap_or_else(|f| eprintln!("{f:?}"));
        //             let (rtv, backbuffer) = create_rtv(&self.device, &resources.swapchain)?;
        //
        //             resources.backbuffer = Some(backbuffer);
        //             resources.backbuffer_rtv = Some(rtv);
        //         }
        //     }
        //     Ok(())
        // }

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
            unsafe {
                // resources
                //     .device
                //     .SetTransform(D3DTS_PROJECTION, IDENTITY_MVP.as_ptr().cast())?;
                // resources
                //     .device
                //     .SetTransform(D3DTS_VIEW, IDENTITY_MVP.as_ptr().cast())?;
                // resources
                //     .device
                //     .SetTransform(D3DTRANSFORMSTATETYPE(256), IDENTITY_MVP.as_ptr().cast())?;
                //
                // let rendertarget = resources.renderbuffer.GetSurfaceLevel(0).unwrap();
                //
                let backbuffer = resources
                    .device
                    .GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO)?;
                //
                // resources.device.SetRenderTarget(0, &rendertarget)?;

                // resources.device.Clear(
                //     0,
                //     std::ptr::null_mut(),
                //     D3DCLEAR_TARGET as u32,
                //     0xFF4d6699,
                //     0.0,
                //     0,
                // )?;

                // resources.device.BeginScene()?;
                //
                // resources.device.SetStreamSource(
                //     0,
                //     &resources.vbo,
                //     0,
                //     std::mem::size_of::<Vertex>() as u32,
                // )?;
                // resources.device.SetVertexDeclaration(&resources.vao)?;
                //
                // resources
                //     .device
                //     .SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE.0 as u32)?;
                // resources
                //     .device
                //     .SetRenderState(D3DRS_CLIPPING, FALSE.0 as u32)?;
                // resources
                //     .device
                //     .SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS.0 as u32)?;
                //
                // resources
                //     .device
                //     .SetRenderState(D3DRS_ZENABLE, FALSE.0 as u32)?;
                // resources
                //     .device
                //     .SetRenderState(D3DRS_LIGHTING, FALSE.0 as u32)?;
                //
                // resources.device.DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1)?;
                //
                // resources.device.EndScene()?;

                resources
                    .filter
                    .frame(
                        &resources.texture,
                        &Viewport {
                            x: 0.0,
                            y: 0.0,
                            mvp: None,
                            output: &backbuffer,
                            size: backbuffer.size()?,
                        },
                        0,
                        None,
                    )
                    .unwrap();

                // resources.device.StretchRect(
                //     &rendertarget,
                //     std::ptr::null_mut(),
                //     &backbuffer,
                //     std::ptr::null_mut(),
                //     D3DTEXF_POINT,
                // )?;
            }

            unsafe {
                resources.device.Present(
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    None,
                    std::ptr::null_mut(),
                )?;
            }

            resources.frame_count += 1;
            Ok(())
        }

        fn resize(&mut self, _w: u32, _h: u32) -> Result<()> {
            Ok(())
        }
    }

    pub fn get_vbo_desc() -> [D3DVERTEXELEMENT9; 3] {
        [
            D3DVERTEXELEMENT9 {
                Stream: 0,
                Offset: 0,
                Type: D3DDECLTYPE_FLOAT3.0 as u8,
                Method: D3DDECLMETHOD_DEFAULT.0 as u8,
                Usage: D3DDECLUSAGE_POSITION.0 as u8,
                UsageIndex: 0,
            },
            D3DVERTEXELEMENT9 {
                Stream: 0,
                Offset: (std::mem::size_of::<f32>() * 4) as u16,
                Type: D3DDECLTYPE_D3DCOLOR.0 as u8,
                Method: D3DDECLMETHOD_DEFAULT.0 as u8,
                Usage: D3DDECLUSAGE_COLOR.0 as u8,
                UsageIndex: 0,
            },
            D3DVERTEXELEMENT9 {
                Stream: 0xFF,
                Offset: 0,
                Type: D3DDECLTYPE_UNUSED.0 as u8,
                Method: 0,
                Usage: 0,
                UsageIndex: 0,
            },
        ]
    }
    fn create_triangle_buffers(
        device: &IDirect3DDevice9,
    ) -> Result<(IDirect3DVertexBuffer9, IDirect3DVertexDeclaration9)> {
        // let vertices = [
        //     Vertex {
        //         position: [0.5f32, -0.5, 0.0],
        //         color: [1.0, 0.0, 0.0],
        //     },
        //     Vertex {
        //         position: [-0.5, -0.5, 0.0],
        //         color: [0.0, 1.0, 0.0],
        //     },
        //     Vertex {
        //         position: [0.0, 0.5, 0.0],
        //         color: [0.0, 0.0, 1.0],
        //     },
        // ];

        const TRIANGLE_VERTICES: [Vertex; 3] = [
            Vertex {
                position: [0.5, -0.5, 0.0, 1.0],
                color: 0xFFFF0000,
            }, // Red
            Vertex {
                position: [-0.5, -0.5, 0.0, 1.0],
                color: 0xFF00FF00,
            }, // Green
            Vertex {
                position: [0.0, 0.5, 0.0, 1.0],
                color: 0xFF0000FF,
            }, // Blue
        ];

        unsafe {
            let mut vb = None;
            device.CreateVertexBuffer(
                (std::mem::size_of::<Vertex>() * 3) as u32,
                0,
                D3DFVF_XYZW | D3DFVF_DIFFUSE,
                D3DPOOL_DEFAULT,
                &mut vb,
                std::ptr::null_mut(),
            )?;

            let vb = vb.unwrap();

            let mut vertices = std::ptr::null_mut();
            vb.Lock(0, 0, &mut vertices, 0)?;
            std::ptr::copy_nonoverlapping(
                TRIANGLE_VERTICES.as_ptr() as *const std::ffi::c_void,
                vertices,
                std::mem::size_of_val(&TRIANGLE_VERTICES),
            );
            vb.Unlock()?;

            let vao = device.CreateVertexDeclaration(get_vbo_desc().as_ptr())?;
            Ok((vb, vao))
        }
    }

    fn create_device(d3d9: &IDirect3D9, hwnd: HWND) -> Result<IDirect3DDevice9> {
        let mut present_params: D3DPRESENT_PARAMETERS = Default::default();
        present_params.BackBufferWidth = WIDTH as u32;
        present_params.BackBufferHeight = HEIGHT as u32;
        present_params.Windowed = TRUE;
        present_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        present_params.hDeviceWindow = hwnd;
        present_params.BackBufferFormat = D3DFMT_UNKNOWN;
        present_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE as u32;

        let mut device = None;

        unsafe {
            // d3d9.CreateDevice(
            //     D3DADAPTER_DEFAULT,
            //     D3DDEVTYPE_HAL,
            //     present_params.hDeviceWindow,
            //     D3DCREATE_HARDWARE_VERTEXPROCESSING as u32,
            //     &mut present_params,
            //     &mut device,
            // )?;

            d3d9.CreateDevice(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                present_params.hDeviceWindow,
                D3DCREATE_HARDWARE_VERTEXPROCESSING as u32,
                &mut present_params,
                &mut device,
            )?;
        }

        Ok(device.unwrap())
    }
}

pub fn main<S: DXSample>(sample: S) -> Result<()> {
    run_sample(sample)?;

    Ok(())
}
