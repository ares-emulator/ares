#![cfg(test)]
use std::ffi::CStr;
use windows::{
    core::*, Win32::Foundation::*, Win32::Graphics::Direct3D::Fxc::*, Win32::Graphics::Direct3D::*,
    Win32::Graphics::Direct3D12::*, Win32::Graphics::Dxgi::Common::*, Win32::Graphics::Dxgi::*,
    Win32::System::LibraryLoader::*, Win32::System::Threading::*,
    Win32::UI::WindowsAndMessaging::*,
};

mod descriptor_heap;

static SHADER: &[u8] = b"struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}\0";

use std::mem::transmute;
use std::path::Path;

pub trait DXSample {
    fn new(filter: impl AsRef<Path>, command_line: &SampleCommandLine) -> Result<Self>
    where
        Self: Sized;

    fn bind_to_window(&mut self, hwnd: &HWND) -> Result<()>;

    fn update(&mut self) {}
    fn render(&mut self) {}
    fn on_key_up(&mut self, _key: u8) {}
    fn on_key_down(&mut self, _key: u8) {}

    fn title(&self) -> String {
        "DXSample".into()
    }

    fn window_size(&self) -> (i32, i32) {
        (600, 800)
    }
}

#[derive(Clone)]
pub struct SampleCommandLine {
    pub use_warp_device: bool,
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
            Some(&mut sample as *mut _ as _),
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
            sample.render();
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

fn get_hardware_adapter(factory: &IDXGIFactory4) -> Result<IDXGIAdapter1> {
    for i in 0.. {
        let adapter = unsafe { factory.EnumAdapters1(i)? };

        let desc = unsafe { adapter.GetDesc1()? };

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE.0 as u32) != DXGI_ADAPTER_FLAG_NONE.0 as u32 {
            // Don't select the Basic Render Driver adapter. If you want a
            // software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see whether the adapter supports Direct3D 12, but don't
        // create the actual device yet.
        if unsafe {
            D3D12CreateDevice(
                &adapter,
                D3D_FEATURE_LEVEL_11_0,
                std::ptr::null_mut::<Option<ID3D12Device>>(),
            )
        }
        .is_ok()
        {
            return Ok(adapter);
        }
    }

    unreachable!()
}

unsafe extern "system" fn debug_log(
    category: D3D12_MESSAGE_CATEGORY,
    severity: D3D12_MESSAGE_SEVERITY,
    _id: D3D12_MESSAGE_ID,
    pdescription: ::windows::core::PCSTR,
    _pcontext: *mut ::core::ffi::c_void,
) {
    unsafe {
        let desc = CStr::from_ptr(pdescription.as_ptr().cast());
        eprintln!("[{severity:?}-{category:?}] {desc:?}")
    }
}

pub mod d3d12_hello_triangle {
    use super::*;
    use crate::hello_triangle::descriptor_heap::CpuStagingHeap;
    use d3d12_descriptor_heap::D3D12DescriptorHeap;
    use librashader_common::{Size, Viewport};
    use librashader_runtime_d3d12::{D3D12InputImage, D3D12OutputView, FilterChainD3D12};
    use std::mem::ManuallyDrop;
    use std::ops::Deref;
    use std::path::Path;

    const FRAME_COUNT: u32 = 2;

    pub struct Sample {
        dxgi_factory: IDXGIFactory4,
        device: ID3D12Device,
        resources: Option<Resources>,
        pub filter: FilterChainD3D12,
        framecount: usize,
    }

    struct Resources {
        command_queue: ID3D12CommandQueue,
        swap_chain: IDXGISwapChain3,
        frame_index: u32,
        render_targets: [ManuallyDrop<ID3D12Resource>; FRAME_COUNT as usize],
        rtv_heap: ID3D12DescriptorHeap,
        rtv_descriptor_size: usize,
        viewport: D3D12_VIEWPORT,
        scissor_rect: RECT,
        command_allocator: ID3D12CommandAllocator,
        root_signature: ID3D12RootSignature,
        pso: ID3D12PipelineState,
        command_list: ID3D12GraphicsCommandList,
        framebuffer: ManuallyDrop<ID3D12Resource>,
        // we need to keep this around to keep the reference alive, even though
        // nothing reads from it
        #[allow(dead_code)]
        vertex_buffer: ID3D12Resource,

        vbv: D3D12_VERTEX_BUFFER_VIEW,
        fence: ID3D12Fence,
        fence_value: u64,
        fence_event: HANDLE,
        frambuffer_heap: D3D12DescriptorHeap<CpuStagingHeap>,
    }

    impl DXSample for Sample {
        fn new(filter: impl AsRef<Path>, command_line: &SampleCommandLine) -> Result<Self> {
            let (dxgi_factory, device) = create_device(command_line)?;
            //
            if let Ok(queue) = device.cast::<ID3D12InfoQueue1>() {
                unsafe {
                    queue
                        .RegisterMessageCallback(
                            Some(debug_log),
                            D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                            std::ptr::null_mut(),
                            &mut 0,
                        )
                        .expect("could not register message callback");
                }
            }

            let filter = unsafe {
                FilterChainD3D12::load_from_path(
                    filter,
                    &device,
                    Some(
                        &librashader_runtime_d3d12::options::FilterChainOptionsD3D12 {
                            disable_cache: true,
                            force_hlsl_pipeline: false,
                            force_no_mipmaps: false,
                            ..Default::default()
                        },
                    ),
                )
                .unwrap()
            };

            Ok(Sample {
                dxgi_factory,
                device,
                resources: None,
                filter,
                framecount: 0,
            })
        }

        fn bind_to_window(&mut self, hwnd: &HWND) -> Result<()> {
            let command_queue: ID3D12CommandQueue = unsafe {
                self.device.CreateCommandQueue(&D3D12_COMMAND_QUEUE_DESC {
                    Type: D3D12_COMMAND_LIST_TYPE_DIRECT,
                    ..Default::default()
                })?
            };

            unsafe {
                command_queue.SetName(w!("MainLoopQueue"))?;
            }

            let (width, height) = self.window_size();

            let swap_chain_desc = DXGI_SWAP_CHAIN_DESC1 {
                BufferCount: FRAME_COUNT,
                Width: width as u32,
                Height: height as u32,
                Format: DXGI_FORMAT_R8G8B8A8_UNORM,
                BufferUsage: DXGI_USAGE_RENDER_TARGET_OUTPUT,
                SwapEffect: DXGI_SWAP_EFFECT_FLIP_DISCARD,
                SampleDesc: DXGI_SAMPLE_DESC {
                    Count: 1,
                    ..Default::default()
                },
                ..Default::default()
            };

            let swap_chain: IDXGISwapChain3 = unsafe {
                self.dxgi_factory.CreateSwapChainForHwnd(
                    &command_queue,
                    *hwnd,
                    &swap_chain_desc,
                    None,
                    None,
                )?
            }
            .cast()?;

            // This sample does not support fullscreen transitions
            unsafe {
                self.dxgi_factory
                    .MakeWindowAssociation(*hwnd, DXGI_MWA_NO_ALT_ENTER)?;
            }

            let frame_index = unsafe { swap_chain.GetCurrentBackBufferIndex() };

            let rtv_heap: ID3D12DescriptorHeap = unsafe {
                self.device
                    .CreateDescriptorHeap(&D3D12_DESCRIPTOR_HEAP_DESC {
                        NumDescriptors: FRAME_COUNT + 1,
                        Type: D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                        ..Default::default()
                    })
            }?;

            let rtv_descriptor_size = unsafe {
                self.device
                    .GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
            } as usize;
            let rtv_handle = unsafe { rtv_heap.GetCPUDescriptorHandleForHeapStart() };

            let render_targets: [ManuallyDrop<ID3D12Resource>; FRAME_COUNT as usize] =
                array_init::try_array_init(|i: usize| -> Result<ManuallyDrop<ID3D12Resource>> {
                    let render_target: ID3D12Resource = unsafe { swap_chain.GetBuffer(i as u32) }?;
                    unsafe {
                        self.device.CreateRenderTargetView(
                            &render_target,
                            None,
                            D3D12_CPU_DESCRIPTOR_HANDLE {
                                ptr: rtv_handle.ptr + i * rtv_descriptor_size,
                            },
                        )
                    };
                    Ok(ManuallyDrop::new(render_target))
                })?;

            let framebuffer: ID3D12Resource = unsafe {
                let render_target: ID3D12Resource = swap_chain.GetBuffer(0)?;
                let mut desc = render_target.GetDesc();
                let mut heapprops = D3D12_HEAP_PROPERTIES::default();
                let mut heappflags = D3D12_HEAP_FLAGS::default();
                render_target.GetHeapProperties(Some(&mut heapprops), Some(&mut heappflags))?;
                desc.Flags &= !D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
                let mut fb = None;
                self.device.CreateCommittedResource(
                    &heapprops,
                    D3D12_HEAP_FLAG_NONE,
                    &desc,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    None,
                    &mut fb,
                )?;

                fb.unwrap()
            };

            unsafe {
                framebuffer.SetName(w!("framebuffer"))?;
            }

            let viewport = D3D12_VIEWPORT {
                TopLeftX: 0.0,
                TopLeftY: 0.0,
                Width: width as f32,
                Height: height as f32,
                MinDepth: D3D12_MIN_DEPTH,
                MaxDepth: D3D12_MAX_DEPTH,
            };

            let scissor_rect = RECT {
                left: 0,
                top: 0,
                right: width,
                bottom: height,
            };

            let command_allocator = unsafe {
                self.device
                    .CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT)
            }?;

            let root_signature = create_root_signature(&self.device)?;
            let pso = create_pipeline_state(&self.device, &root_signature)?;

            let command_list: ID3D12GraphicsCommandList = unsafe {
                self.device.CreateCommandList(
                    0,
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    &command_allocator,
                    &pso,
                )
            }?;
            unsafe {
                command_list.Close()?;
            };

            let aspect_ratio = width as f32 / height as f32;

            let (vertex_buffer, vbv) = create_vertex_buffer(&self.device, aspect_ratio)?;

            let fence = unsafe { self.device.CreateFence(0, D3D12_FENCE_FLAG_NONE) }?;

            let fence_value = 1;

            let fence_event = unsafe { CreateEventA(None, false, false, None)? };

            self.resources = Some(Resources {
                command_queue,
                swap_chain,
                frame_index,
                render_targets,
                rtv_heap,
                rtv_descriptor_size,
                viewport,
                scissor_rect,
                command_allocator,
                root_signature,
                pso,
                command_list,
                framebuffer: ManuallyDrop::new(framebuffer),
                vertex_buffer,
                vbv,
                fence,
                fence_value,
                fence_event,
                frambuffer_heap: unsafe { D3D12DescriptorHeap::new(&self.device, 1024).unwrap() },
            });

            Ok(())
        }

        fn title(&self) -> String {
            "librashader DirectX 12".into()
        }

        fn window_size(&self) -> (i32, i32) {
            (800, 600)
        }

        fn render(&mut self) {
            if let Some(resources) = &mut self.resources {
                let srv = resources.frambuffer_heap.allocate_descriptor().unwrap();

                unsafe {
                    self.device.CreateShaderResourceView(
                        resources.framebuffer.deref(),
                        Some(&D3D12_SHADER_RESOURCE_VIEW_DESC {
                            Format: DXGI_FORMAT_R8G8B8A8_UNORM,
                            ViewDimension: D3D12_SRV_DIMENSION_TEXTURE2D,
                            Shader4ComponentMapping: D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                            Anonymous: D3D12_SHADER_RESOURCE_VIEW_DESC_0 {
                                Texture2D: D3D12_TEX2D_SRV {
                                    MipLevels: u32::MAX,
                                    ..Default::default()
                                },
                            },
                        }),
                        *srv.as_ref(),
                    )
                }

                populate_command_list(resources, &mut self.filter, self.framecount, *srv.as_ref())
                    .unwrap();

                // Execute the command list.
                let command_list: Option<ID3D12CommandList> = resources.command_list.cast().ok();
                unsafe { resources.command_queue.ExecuteCommandLists(&[command_list]) };

                // Present the frame.
                unsafe { resources.swap_chain.Present(1, DXGI_PRESENT::default()) }
                    .ok()
                    .unwrap();

                wait_for_previous_frame(resources);
                self.framecount += 1;
            }
        }
    }

    fn populate_command_list(
        resources: &mut Resources,
        filter: &mut FilterChainD3D12,
        frame_count: usize,
        framebuffer: D3D12_CPU_DESCRIPTOR_HANDLE,
    ) -> Result<()> {
        // Command list allocators can only be reset when the associated
        // command lists have finished execution on the GPU; apps should use
        // fences to determine GPU execution progress.
        unsafe {
            resources.command_allocator.Reset()?;
        }

        let command_list = &resources.command_list;

        // However, when ExecuteCommandList() is called on a particular
        // command list, that command list can then be reset at any time and
        // must be before re-recording.
        unsafe {
            command_list.Reset(&resources.command_allocator, &resources.pso)?;
        }

        // Set necessary state.
        unsafe {
            command_list.SetGraphicsRootSignature(&resources.root_signature);
            command_list.RSSetViewports(&[resources.viewport]);
            command_list.RSSetScissorRects(&[resources.scissor_rect]);
        }

        // Indicate that the back buffer will be used as a render target.
        let barrier = transition_barrier(
            &resources.render_targets[resources.frame_index as usize],
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
        );
        unsafe { command_list.ResourceBarrier(&[barrier]) };

        let rtv_handle = D3D12_CPU_DESCRIPTOR_HANDLE {
            ptr: unsafe { resources.rtv_heap.GetCPUDescriptorHandleForHeapStart() }.ptr
                + resources.frame_index as usize * resources.rtv_descriptor_size,
        };

        unsafe { command_list.OMSetRenderTargets(1, Some(&rtv_handle), false, None) };

        // Record commands.
        unsafe {
            command_list.ClearRenderTargetView(rtv_handle, &[0.3, 0.4, 0.6, 1.0], Some(&[]));
            command_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            command_list.IASetVertexBuffers(0, Some(&[resources.vbv]));
            command_list.DrawInstanced(3, 1, 0, 0);

            command_list.ResourceBarrier(&[transition_barrier(
                &resources.render_targets[resources.frame_index as usize],
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_COPY_SOURCE,
            )]);

            command_list.ResourceBarrier(&[transition_barrier(
                &resources.framebuffer,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_COPY_DEST,
            )]);
        }

        unsafe {
            command_list.CopyResource(
                &*resources.framebuffer,
                &*resources.render_targets[resources.frame_index as usize],
            );
            command_list.ResourceBarrier(&[transition_barrier(
                &resources.framebuffer,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            )]);

            command_list.ResourceBarrier(&[transition_barrier(
                &resources.render_targets[resources.frame_index as usize],
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
            )]);

            filter
                .frame(
                    command_list,
                    D3D12InputImage::External {
                        resource: resources.framebuffer.clone(),
                        descriptor: framebuffer,
                    },
                    &Viewport {
                        x: 0.0,
                        y: 0.0,
                        mvp: None,
                        output: D3D12OutputView::new_from_raw(
                            rtv_handle,
                            Size::new(
                                resources.viewport.Width as u32,
                                resources.viewport.Height as u32,
                            ),
                            DXGI_FORMAT_R8G8B8A8_UNORM,
                        ),
                        size: Size::new(
                            resources.viewport.Width as u32,
                            resources.viewport.Height as u32,
                        ),
                    },
                    frame_count,
                    None,
                )
                .unwrap();

            command_list.ResourceBarrier(&[transition_barrier(
                &resources.render_targets[resources.frame_index as usize],
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT,
            )]);
        }

        unsafe { command_list.Close() }
    }

    fn transition_barrier(
        resource: &ManuallyDrop<ID3D12Resource>,
        state_before: D3D12_RESOURCE_STATES,
        state_after: D3D12_RESOURCE_STATES,
    ) -> D3D12_RESOURCE_BARRIER {
        D3D12_RESOURCE_BARRIER {
            Type: D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            Flags: D3D12_RESOURCE_BARRIER_FLAG_NONE,
            Anonymous: D3D12_RESOURCE_BARRIER_0 {
                Transition: ManuallyDrop::new(D3D12_RESOURCE_TRANSITION_BARRIER {
                    pResource: unsafe { std::mem::transmute_copy(resource) },
                    StateBefore: state_before,
                    StateAfter: state_after,
                    Subresource: D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                }),
            },
        }
    }

    fn create_device(command_line: &SampleCommandLine) -> Result<(IDXGIFactory4, ID3D12Device)> {
        unsafe {
            let mut debug: Option<ID3D12Debug> = None;
            if let Some(debug) = D3D12GetDebugInterface(&mut debug).ok().and(debug) {
                eprintln!("enabling debug");
                debug.EnableDebugLayer();
            }
        }

        let dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;

        let dxgi_factory: IDXGIFactory4 = unsafe { CreateDXGIFactory2(dxgi_factory_flags) }?;

        let adapter = if command_line.use_warp_device {
            unsafe { dxgi_factory.EnumWarpAdapter() }
        } else {
            get_hardware_adapter(&dxgi_factory)
        }?;

        let mut device: Option<ID3D12Device> = None;
        unsafe { D3D12CreateDevice(&adapter, D3D_FEATURE_LEVEL_12_1, &mut device) }?;
        Ok((dxgi_factory, device.unwrap()))
    }

    fn create_root_signature(device: &ID3D12Device) -> Result<ID3D12RootSignature> {
        let desc = D3D12_ROOT_SIGNATURE_DESC {
            Flags: D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
            ..Default::default()
        };

        let mut signature = None;

        let signature = unsafe {
            D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &mut signature, None)
        }
        .map(|()| signature.unwrap())?;

        unsafe {
            device.CreateRootSignature(
                0,
                std::slice::from_raw_parts(
                    signature.GetBufferPointer() as _,
                    signature.GetBufferSize(),
                ),
            )
        }
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

    fn create_pipeline_state(
        device: &ID3D12Device,
        root_signature: &ID3D12RootSignature,
    ) -> Result<ID3D12PipelineState> {
        let _compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        let vertex_shader = compile_shader(SHADER, b"VSMain\0", b"vs_5_0\0")?;
        let pixel_shader = compile_shader(SHADER, b"PSMain\0", b"ps_5_0\0")?;

        let mut input_element_descs: [D3D12_INPUT_ELEMENT_DESC; 2] = [
            D3D12_INPUT_ELEMENT_DESC {
                SemanticName: s!("POSITION"),
                SemanticIndex: 0,
                Format: DXGI_FORMAT_R32G32B32_FLOAT,
                InputSlot: 0,
                AlignedByteOffset: 0,
                InputSlotClass: D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
            D3D12_INPUT_ELEMENT_DESC {
                SemanticName: s!("COLOR"),
                SemanticIndex: 0,
                Format: DXGI_FORMAT_R32G32B32A32_FLOAT,
                InputSlot: 0,
                AlignedByteOffset: 12,
                InputSlotClass: D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                InstanceDataStepRate: 0,
            },
        ];

        let mut desc = D3D12_GRAPHICS_PIPELINE_STATE_DESC {
            InputLayout: D3D12_INPUT_LAYOUT_DESC {
                pInputElementDescs: input_element_descs.as_mut_ptr(),
                NumElements: input_element_descs.len() as u32,
            },
            pRootSignature: ManuallyDrop::new(Some(root_signature.clone())),
            VS: D3D12_SHADER_BYTECODE {
                pShaderBytecode: unsafe { vertex_shader.GetBufferPointer() },
                BytecodeLength: unsafe { vertex_shader.GetBufferSize() },
            },
            PS: D3D12_SHADER_BYTECODE {
                pShaderBytecode: unsafe { pixel_shader.GetBufferPointer() },
                BytecodeLength: unsafe { pixel_shader.GetBufferSize() },
            },
            RasterizerState: D3D12_RASTERIZER_DESC {
                FillMode: D3D12_FILL_MODE_SOLID,
                CullMode: D3D12_CULL_MODE_NONE,
                ..Default::default()
            },
            BlendState: D3D12_BLEND_DESC {
                AlphaToCoverageEnable: false.into(),
                IndependentBlendEnable: false.into(),
                RenderTarget: [
                    D3D12_RENDER_TARGET_BLEND_DESC {
                        BlendEnable: false.into(),
                        LogicOpEnable: false.into(),
                        SrcBlend: D3D12_BLEND_ONE,
                        DestBlend: D3D12_BLEND_ZERO,
                        BlendOp: D3D12_BLEND_OP_ADD,
                        SrcBlendAlpha: D3D12_BLEND_ONE,
                        DestBlendAlpha: D3D12_BLEND_ZERO,
                        BlendOpAlpha: D3D12_BLEND_OP_ADD,
                        LogicOp: D3D12_LOGIC_OP_NOOP,
                        RenderTargetWriteMask: D3D12_COLOR_WRITE_ENABLE_ALL.0 as u8,
                    },
                    D3D12_RENDER_TARGET_BLEND_DESC::default(),
                    D3D12_RENDER_TARGET_BLEND_DESC::default(),
                    D3D12_RENDER_TARGET_BLEND_DESC::default(),
                    D3D12_RENDER_TARGET_BLEND_DESC::default(),
                    D3D12_RENDER_TARGET_BLEND_DESC::default(),
                    D3D12_RENDER_TARGET_BLEND_DESC::default(),
                    D3D12_RENDER_TARGET_BLEND_DESC::default(),
                ],
            },
            DepthStencilState: D3D12_DEPTH_STENCIL_DESC::default(),
            SampleMask: u32::max_value(),
            PrimitiveTopologyType: D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            NumRenderTargets: 1,
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                ..Default::default()
            },
            ..Default::default()
        };
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        unsafe { device.CreateGraphicsPipelineState(&desc) }
    }

    fn create_vertex_buffer(
        device: &ID3D12Device,
        _aspect_ratio: f32,
    ) -> Result<(ID3D12Resource, D3D12_VERTEX_BUFFER_VIEW)> {
        let vertices = [
            Vertex {
                position: [0.5f32, -0.5, 0.0],
                color: [1.0, 0.0, 0.0, 1.0],
            },
            Vertex {
                position: [-0.5, -0.5, 0.0],
                color: [0.0, 1.0, 0.0, 1.0],
            },
            Vertex {
                position: [0.0, 0.5, 0.0],
                color: [0.0, 0.0, 1.0, 1.0],
            },
        ];

        // Note: using upload heaps to transfer static data like vert buffers is
        // not recommended. Every time the GPU needs it, the upload heap will be
        // marshalled over. Please read up on Default Heap usage. An upload heap
        // is used here for code simplicity and because there are very few verts
        // to actually transfer.
        let mut vertex_buffer: Option<ID3D12Resource> = None;
        unsafe {
            device.CreateCommittedResource(
                &D3D12_HEAP_PROPERTIES {
                    Type: D3D12_HEAP_TYPE_UPLOAD,
                    ..Default::default()
                },
                D3D12_HEAP_FLAG_NONE,
                &D3D12_RESOURCE_DESC {
                    Dimension: D3D12_RESOURCE_DIMENSION_BUFFER,
                    Width: std::mem::size_of_val(&vertices) as u64,
                    Height: 1,
                    DepthOrArraySize: 1,
                    MipLevels: 1,
                    SampleDesc: DXGI_SAMPLE_DESC {
                        Count: 1,
                        Quality: 0,
                    },
                    Layout: D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                    ..Default::default()
                },
                D3D12_RESOURCE_STATE_GENERIC_READ,
                None,
                &mut vertex_buffer,
            )?
        };
        let vertex_buffer = vertex_buffer.unwrap();

        // Copy the triangle data to the vertex buffer.
        unsafe {
            let mut data = std::ptr::null_mut();
            vertex_buffer.Map(0, None, Some(&mut data))?;
            std::ptr::copy_nonoverlapping(vertices.as_ptr(), data as *mut Vertex, vertices.len());
            vertex_buffer.Unmap(0, None);
        }

        let vbv = D3D12_VERTEX_BUFFER_VIEW {
            BufferLocation: unsafe { vertex_buffer.GetGPUVirtualAddress() },
            StrideInBytes: std::mem::size_of::<Vertex>() as u32,
            SizeInBytes: std::mem::size_of_val(&vertices) as u32,
        };

        Ok((vertex_buffer, vbv))
    }

    #[repr(C)]
    struct Vertex {
        position: [f32; 3],
        color: [f32; 4],
    }

    fn wait_for_previous_frame(resources: &mut Resources) {
        // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST
        // PRACTICE. This is code implemented as such for simplicity. The
        // D3D12HelloFrameBuffering sample illustrates how to use fences for
        // efficient resource usage and to maximize GPU utilization.

        // Signal and increment the fence value.
        let fence = resources.fence_value;

        unsafe { resources.command_queue.Signal(&resources.fence, fence) }
            .ok()
            .unwrap();

        resources.fence_value += 1;

        // Wait until the previous frame is finished.
        if unsafe { resources.fence.GetCompletedValue() } < fence {
            unsafe {
                resources
                    .fence
                    .SetEventOnCompletion(fence, resources.fence_event)
            }
            .ok()
            .unwrap();

            unsafe { WaitForSingleObject(resources.fence_event, INFINITE) };
        }

        resources.frame_index = unsafe { resources.swap_chain.GetCurrentBackBufferIndex() };
    }
}

pub(crate) fn main<S: DXSample>(sample: S) -> Result<()> {
    run_sample(sample)?;

    Ok(())
}
