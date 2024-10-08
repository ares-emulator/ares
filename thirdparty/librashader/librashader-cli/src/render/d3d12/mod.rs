mod descriptor_heap;
mod util;

use crate::render::d3d12::descriptor_heap::{CpuStagingHeap, RenderTargetHeap};
use crate::render::{CommonFrameOptions, RenderTest};
use anyhow::anyhow;
use d3d12_descriptor_heap::{D3D12DescriptorHeap, D3D12DescriptorHeapSlot};
use image::RgbaImage;
use librashader::presets::ShaderPreset;
use librashader::runtime::d3d12::{D3D12OutputView, FilterChain, FilterChainOptions, FrameOptions};
use librashader::runtime::{FilterChainParameters, RuntimeParameters};
use librashader::runtime::{Size, Viewport};
use librashader_runtime::image::{Image, PixelFormat, UVDirection, BGRA8};
use std::path::Path;
use windows::core::Interface;
use windows::Win32::Foundation::CloseHandle;
use windows::Win32::Graphics::Direct3D::{D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_12_1};
use windows::Win32::Graphics::Direct3D12::{
    D3D12CreateDevice, ID3D12CommandAllocator, ID3D12CommandQueue, ID3D12Device, ID3D12Fence,
    ID3D12GraphicsCommandList, ID3D12Resource, D3D12_COMMAND_LIST_TYPE_DIRECT,
    D3D12_COMMAND_QUEUE_DESC, D3D12_COMMAND_QUEUE_FLAG_NONE, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    D3D12_FENCE_FLAG_NONE, D3D12_HEAP_FLAG_NONE, D3D12_HEAP_PROPERTIES, D3D12_HEAP_TYPE_CUSTOM,
    D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_TYPE_UPLOAD, D3D12_MEMORY_POOL_L0,
    D3D12_MEMORY_POOL_UNKNOWN, D3D12_PLACED_SUBRESOURCE_FOOTPRINT, D3D12_RESOURCE_DESC,
    D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
    D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON,
    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_SHADER_RESOURCE_VIEW_DESC, D3D12_SHADER_RESOURCE_VIEW_DESC_0,
    D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_SUBRESOURCE_DATA, D3D12_TEX2D_SRV,
    D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
};
use windows::Win32::Graphics::Dxgi::Common::{DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_SAMPLE_DESC};
use windows::Win32::Graphics::Dxgi::{
    CreateDXGIFactory2, IDXGIAdapter1, IDXGIFactory4, DXGI_ADAPTER_FLAG_NONE,
    DXGI_ADAPTER_FLAG_SOFTWARE, DXGI_CREATE_FACTORY_DEBUG,
};
use windows::Win32::System::Threading::{CreateEventA, WaitForSingleObject, INFINITE};

pub struct Direct3D12 {
    device: ID3D12Device,
    _cpu_heap: D3D12DescriptorHeap<CpuStagingHeap>,
    rtv_heap: D3D12DescriptorHeap<RenderTargetHeap>,

    texture: ID3D12Resource,
    _heap_slot: D3D12DescriptorHeapSlot<CpuStagingHeap>,
    command_pool: ID3D12CommandAllocator,
    queue: ID3D12CommandQueue,
    image: Image<BGRA8>,
}

impl RenderTest for Direct3D12 {
    fn new(path: &Path) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        Direct3D12::new(path)
    }

    fn image_size(&self) -> Size<u32> {
        self.image.size
    }
    fn render_with_preset_and_params(
        &mut self,
        preset: ShaderPreset,
        frame_count: usize,
        output_size: Option<Size<u32>>,
        param_setter: Option<&dyn Fn(&RuntimeParameters)>,
        frame_options: Option<CommonFrameOptions>,
    ) -> anyhow::Result<image::RgbaImage> {
        unsafe {
            let descriptor = self.rtv_heap.allocate_descriptor()?;

            let cmd: ID3D12GraphicsCommandList = self.device.CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                &self.command_pool,
                None,
            )?;

            let fence_event = CreateEventA(None, false, false, None)?;
            let fence: ID3D12Fence = self.device.CreateFence(0, D3D12_FENCE_FLAG_NONE)?;

            let mut filter_chain = FilterChain::load_from_preset(
                preset,
                &self.device,
                Some(&FilterChainOptions {
                    force_hlsl_pipeline: false,
                    force_no_mipmaps: false,
                    disable_cache: false,
                }),
            )?;

            if let Some(setter) = param_setter {
                setter(filter_chain.parameters());
            }

            let output_size = output_size.unwrap_or(self.image.size);
            let mut output_texture = None;
            let desc = D3D12_RESOURCE_DESC {
                Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                Alignment: 0,
                Width: output_size.width as u64,
                Height: output_size.height,
                DepthOrArraySize: 1,
                MipLevels: 1,
                Format: DXGI_FORMAT_B8G8R8A8_UNORM,
                SampleDesc: DXGI_SAMPLE_DESC {
                    Count: 1,
                    Quality: 0,
                },
                Layout: Default::default(),
                Flags: D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
            };

            // We only need to draw one frame, so don't worry about the performance impact
            // of this heap.
            self.device.CreateCommittedResource(
                &D3D12_HEAP_PROPERTIES {
                    Type: D3D12_HEAP_TYPE_CUSTOM,
                    CPUPageProperty: D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE,
                    MemoryPoolPreference: D3D12_MEMORY_POOL_L0,
                    CreationNodeMask: 1,
                    VisibleNodeMask: 1,
                },
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_RENDER_TARGET,
                None,
                &mut output_texture,
            )?;

            let output_texture: ID3D12Resource =
                output_texture.ok_or_else(|| anyhow!("Failed to allocate resource"))?;

            self.device
                .CreateRenderTargetView(&output_texture, None, *descriptor.as_ref());

            let viewport = Viewport::new_render_target_sized_origin(
                D3D12OutputView::new_from_raw(
                    *descriptor.as_ref(),
                    output_size,
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                ),
                None,
            )?;

            let options = frame_options.map(|options| FrameOptions {
                clear_history: options.clear_history,
                frame_direction: options.frame_direction,
                rotation: options.rotation,
                total_subframes: options.total_subframes,
                current_subframe: options.current_subframe,
            });

            let image = self.texture.to_ref();

            for frame in 0..=frame_count {
                filter_chain.frame(&cmd, image.into(), &viewport, frame, options.as_ref())?;
            }

            cmd.Close()?;
            self.queue.ExecuteCommandLists(&[Some(cmd.cast()?)]);
            self.queue.Signal(&fence, 1)?;

            if fence.GetCompletedValue() < 1 {
                fence.SetEventOnCompletion(1, fence_event)?;
                WaitForSingleObject(fence_event, INFINITE);
                CloseHandle(fence_event)?;
            };

            let mut buffer = vec![0u8; (output_size.height * output_size.width) as usize * 4];

            output_texture.ReadFromSubresource(
                buffer.as_mut_ptr().cast(),
                4 * output_size.width,
                0,
                0,
                None,
            )?;

            BGRA8::convert(&mut buffer);

            let image =
                RgbaImage::from_raw(output_size.width, output_size.height, Vec::from(buffer))
                    .ok_or(anyhow!("Unable to create image from data"))?;

            Ok(image)
        }
    }
}

impl Direct3D12 {
    pub fn new(image_path: &Path) -> anyhow::Result<Self> {
        let device = Self::create_device()?;
        let mut heap = unsafe { D3D12DescriptorHeap::new(&device, 8)? };
        let rtv_heap = unsafe { D3D12DescriptorHeap::new(&device, 16)? };

        unsafe {
            let command_pool: ID3D12CommandAllocator =
                device.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT)?;

            let queue: ID3D12CommandQueue =
                device.CreateCommandQueue(&D3D12_COMMAND_QUEUE_DESC {
                    Type: D3D12_COMMAND_LIST_TYPE_DIRECT,
                    Priority: 0,
                    Flags: D3D12_COMMAND_QUEUE_FLAG_NONE,
                    NodeMask: 0,
                })?;
            let (image, texture, heap_slot) =
                Self::load_image(&device, &command_pool, &queue, &mut heap, image_path)?;

            Ok(Self {
                device,
                _cpu_heap: heap,
                rtv_heap,
                texture,
                _heap_slot: heap_slot,
                command_pool,
                image,
                queue,
            })
        }
    }

    fn create_device() -> anyhow::Result<ID3D12Device> {
        let dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
        let dxgi_factory: IDXGIFactory4 = unsafe { CreateDXGIFactory2(dxgi_factory_flags) }?;

        let adapter = Self::get_hardware_adapter(&dxgi_factory)?;

        let mut device: Option<ID3D12Device> = None;
        unsafe { D3D12CreateDevice(&adapter, D3D_FEATURE_LEVEL_12_1, &mut device) }?;
        let device = device.ok_or(anyhow!("Failed to initialize D3D12 device"))?;

        Ok(device)
    }

    fn load_image(
        device: &ID3D12Device,
        command_pool: &ID3D12CommandAllocator,
        queue: &ID3D12CommandQueue,
        heap: &mut D3D12DescriptorHeap<CpuStagingHeap>,
        path: &Path,
    ) -> anyhow::Result<(
        Image<BGRA8>,
        ID3D12Resource,
        D3D12DescriptorHeapSlot<CpuStagingHeap>,
    )> {
        // 1 time queue infrastructure for lut uploads
        let image: Image<BGRA8> = Image::load(path, UVDirection::TopLeft)?;

        let desc = D3D12_RESOURCE_DESC {
            Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            Alignment: 0,
            Width: image.size.width as u64,
            Height: image.size.height,
            DepthOrArraySize: 1,
            MipLevels: 1,
            Format: DXGI_FORMAT_B8G8R8A8_UNORM,
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                Quality: 0,
            },
            Layout: Default::default(),
            Flags: Default::default(),
        };

        let descriptor = heap.allocate_descriptor()?;

        // create handles on GPU
        let mut resource: Option<ID3D12Resource> = None;
        unsafe {
            let cmd: ID3D12GraphicsCommandList = device.CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                &command_pool.clone(),
                None,
            )?;
            let fence_event = CreateEventA(None, false, false, None)?;
            let fence: ID3D12Fence = device.CreateFence(0, D3D12_FENCE_FLAG_NONE)?;

            device.CreateCommittedResource(
                &D3D12_HEAP_PROPERTIES {
                    Type: D3D12_HEAP_TYPE_DEFAULT,
                    CPUPageProperty: D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                    MemoryPoolPreference: D3D12_MEMORY_POOL_UNKNOWN,
                    CreationNodeMask: 1,
                    VisibleNodeMask: 1,
                },
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                None,
                &mut resource,
            )?;

            let resource = resource.ok_or_else(|| anyhow!("Failed to allocate resource"))?;
            let srv_desc = D3D12_SHADER_RESOURCE_VIEW_DESC {
                Format: desc.Format,
                ViewDimension: D3D12_SRV_DIMENSION_TEXTURE2D,
                Shader4ComponentMapping: D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                Anonymous: D3D12_SHADER_RESOURCE_VIEW_DESC_0 {
                    Texture2D: D3D12_TEX2D_SRV {
                        MipLevels: u32::MAX,
                        ..Default::default()
                    },
                },
            };

            device.CreateShaderResourceView(&resource, Some(&srv_desc), *descriptor.as_ref());

            let mut buffer_desc = D3D12_RESOURCE_DESC {
                Dimension: D3D12_RESOURCE_DIMENSION_BUFFER,
                ..Default::default()
            };

            let mut layout = D3D12_PLACED_SUBRESOURCE_FOOTPRINT::default();
            let mut total = 0;
            // texture upload
            device.GetCopyableFootprints(
                &desc,
                0,
                1,
                0,
                Some(&mut layout),
                None,
                None,
                Some(&mut total),
            );

            buffer_desc.Width = total;
            buffer_desc.Height = 1;
            buffer_desc.DepthOrArraySize = 1;
            buffer_desc.MipLevels = 1;
            buffer_desc.SampleDesc.Count = 1;
            buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            let mut upload: Option<ID3D12Resource> = None;

            device.CreateCommittedResource(
                &D3D12_HEAP_PROPERTIES {
                    Type: D3D12_HEAP_TYPE_UPLOAD,
                    CPUPageProperty: D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                    MemoryPoolPreference: D3D12_MEMORY_POOL_UNKNOWN,
                    CreationNodeMask: 1,
                    VisibleNodeMask: 1,
                },
                D3D12_HEAP_FLAG_NONE,
                &buffer_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                None,
                &mut upload,
            )?;

            let upload = upload.ok_or_else(|| anyhow!("Failed to allocate copy texture"))?;

            let subresource = [D3D12_SUBRESOURCE_DATA {
                pData: image.bytes.as_ptr().cast(),
                RowPitch: 4 * image.size.width as isize,
                SlicePitch: (4 * image.size.width * image.size.height) as isize,
            }];

            util::d3d12_resource_transition(
                &cmd,
                &resource,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_COPY_DEST,
            );

            util::d3d12_update_subresources(&cmd, &resource, &upload, 0, 0, 1, &subresource)?;

            util::d3d12_resource_transition(
                &cmd,
                &resource,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            );

            cmd.Close()?;
            queue.ExecuteCommandLists(&[Some(cmd.cast()?)]);
            queue.Signal(&fence, 1)?;

            if fence.GetCompletedValue() < 1 {
                fence.SetEventOnCompletion(1, fence_event)?;
                WaitForSingleObject(fence_event, INFINITE);
                CloseHandle(fence_event)?;
            }

            Ok((image, resource, descriptor))
        }
    }

    fn get_hardware_adapter(factory: &IDXGIFactory4) -> windows::core::Result<IDXGIAdapter1> {
        for i in 0.. {
            let adapter = unsafe { factory.EnumAdapters1(i)? };

            let desc = unsafe { adapter.GetDesc1()? };

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE.0 as u32) != DXGI_ADAPTER_FLAG_NONE.0 as u32
            {
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

        // Fallback to warp
        unsafe { factory.EnumWarpAdapter() }
    }
}
