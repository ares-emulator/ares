use crate::descriptor_heap::{CpuStagingHeap, RenderTargetHeap};
use crate::error::FilterChainError;
use crate::resource::{OutlivesFrame, ResourceHandleStrategy};
use crate::texture::{D3D12OutputView, InputTexture};
use crate::util::d3d12_get_closest_format;
use crate::{error, util};
use d3d12_descriptor_heap::D3D12DescriptorHeap;
use gpu_allocator::d3d12::{
    Allocator, Resource, ResourceCategory, ResourceCreateDesc, ResourceStateOrBarrierLayout,
    ResourceType,
};
use gpu_allocator::MemoryLocation;
use librashader_common::{FilterMode, ImageFormat, Size, WrapMode};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::{MipmapSize, ScaleFramebuffer, ViewportSize};
use parking_lot::Mutex;
use std::mem::ManuallyDrop;
use std::sync::Arc;
use windows::core::Interface;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, ID3D12Resource, D3D12_BOX,
    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, D3D12_FEATURE_DATA_FORMAT_SUPPORT,
    D3D12_FORMAT_SUPPORT1_MIP, D3D12_FORMAT_SUPPORT1_RENDER_TARGET,
    D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE, D3D12_FORMAT_SUPPORT1_TEXTURE2D,
    D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD, D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE,
    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_DESC,
    D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST,
    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_SHADER_RESOURCE_VIEW_DESC,
    D3D12_SHADER_RESOURCE_VIEW_DESC_0, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_TEX2D_SRV,
    D3D12_TEXTURE_COPY_LOCATION, D3D12_TEXTURE_COPY_LOCATION_0,
    D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
};
use windows::Win32::Graphics::Dxgi::Common::{DXGI_FORMAT, DXGI_SAMPLE_DESC};

#[derive(Debug)]
pub(crate) struct OwnedImage {
    pub(crate) handle: ManuallyDrop<Resource>,
    pub(crate) resource: ManuallyDrop<ID3D12Resource>,
    pub(crate) size: Size<u32>,
    pub(crate) format: DXGI_FORMAT,
    pub(crate) max_mipmap: u16,
    device: ID3D12Device,
    allocator: Arc<Mutex<Allocator>>,
}

static CLEAR: &[f32; 4] = &[0.0, 0.0, 0.0, 0.0];

impl OwnedImage {
    pub fn get_format_support(
        device: &ID3D12Device,
        format: DXGI_FORMAT,
        mipmap: bool,
    ) -> DXGI_FORMAT {
        let mut format_support = D3D12_FEATURE_DATA_FORMAT_SUPPORT {
            Format: format,
            Support1: D3D12_FORMAT_SUPPORT1_TEXTURE2D
                | D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE
                | D3D12_FORMAT_SUPPORT1_RENDER_TARGET,
            ..Default::default()
        };

        if mipmap {
            format_support.Support1 |= D3D12_FORMAT_SUPPORT1_MIP;
            format_support.Support2 |=
                D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
        }

        d3d12_get_closest_format(device, format_support)
    }

    pub fn new(
        device: &ID3D12Device,
        allocator: &Arc<Mutex<Allocator>>,
        size: Size<u32>,
        format: DXGI_FORMAT,
        mipmap: bool,
    ) -> error::Result<OwnedImage> {
        let miplevels = if mipmap {
            size.calculate_miplevels()
        } else {
            1
        };
        let mut desc = D3D12_RESOURCE_DESC {
            Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            Alignment: 0,
            Width: size.width as u64,
            Height: size.height,
            DepthOrArraySize: 1,
            MipLevels: miplevels as u16,
            Format: format.into(),
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                Quality: 0,
            },
            Layout: Default::default(),
            Flags: D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        };

        let mut format_support = D3D12_FEATURE_DATA_FORMAT_SUPPORT {
            Format: desc.Format,
            Support1: D3D12_FORMAT_SUPPORT1_TEXTURE2D
                | D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE
                | D3D12_FORMAT_SUPPORT1_RENDER_TARGET,
            ..Default::default()
        };

        if mipmap {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            format_support.Support1 |= D3D12_FORMAT_SUPPORT1_MIP;
            format_support.Support2 |=
                D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
        }

        desc.Format = d3d12_get_closest_format(device, format_support);

        let allocator_resource = allocator.lock().create_resource(&ResourceCreateDesc {
            name: "ownedimage",
            memory_location: MemoryLocation::GpuOnly,
            resource_category: ResourceCategory::RtvDsvTexture,
            resource_desc: &desc,
            castable_formats: &[],
            clear_value: None,
            initial_state_or_layout: ResourceStateOrBarrierLayout::ResourceState(
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            ),
            resource_type: &ResourceType::Placed,
        })?;

        // let mut resource: Option<ID3D12Resource> = None;
        // unsafe {
        //     device.CreateCommittedResource(
        //         &D3D12_HEAP_PROPERTIES {
        //             Type: D3D12_HEAP_TYPE_DEFAULT,
        //             CPUPageProperty: D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        //             MemoryPoolPreference: D3D12_MEMORY_POOL_UNKNOWN,
        //             CreationNodeMask: 1,
        //             VisibleNodeMask: 1,
        //         },
        //         D3D12_HEAP_FLAG_NONE,
        //         &desc,
        //         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        //         None,
        //         &mut resource,
        //     )?;
        // }
        // assume_d3d12_init!(resource, "CreateCommittedResource");

        let resource = ManuallyDrop::new(allocator_resource.resource().clone());
        Ok(OwnedImage {
            handle: ManuallyDrop::new(allocator_resource),
            resource,
            size,
            format: desc.Format,
            device: device.clone(),
            max_mipmap: miplevels as u16,
            allocator: Arc::clone(&allocator),
        })
    }

    /// SAFETY: self must fit the source image
    /// source must be in D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    pub unsafe fn copy_from(
        &self,
        cmd: &ID3D12GraphicsCommandList,
        input: &InputTexture,
    ) -> error::Result<()> {
        let barriers = [
            util::d3d12_get_resource_transition_subresource::<OutlivesFrame, _>(
                &input.resource,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            ),
            util::d3d12_get_resource_transition_subresource::<OutlivesFrame, _>(
                &self.resource,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            ),
        ];

        unsafe {
            cmd.ResourceBarrier(&barriers);

            let dst = D3D12_TEXTURE_COPY_LOCATION {
                pResource: OutlivesFrame::obtain(&self.resource),
                Type: D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                Anonymous: D3D12_TEXTURE_COPY_LOCATION_0 {
                    SubresourceIndex: 0,
                },
            };

            let src = D3D12_TEXTURE_COPY_LOCATION {
                pResource: OutlivesFrame::obtain(&input.resource),
                Type: D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                Anonymous: D3D12_TEXTURE_COPY_LOCATION_0 {
                    SubresourceIndex: 0,
                },
            };

            cmd.CopyTextureRegion(
                &dst,
                0,
                0,
                0,
                &src,
                Some(&D3D12_BOX {
                    left: 0,
                    top: 0,
                    front: 0,
                    right: input.size.width,
                    bottom: input.size.height,
                    back: 1,
                }),
            );
        }

        let barriers = [
            util::d3d12_get_resource_transition_subresource::<OutlivesFrame, _>(
                &input.resource,
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            ),
            util::d3d12_get_resource_transition_subresource::<OutlivesFrame, _>(
                &self.resource,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            ),
        ];

        unsafe {
            cmd.ResourceBarrier(&barriers);
        }

        Ok(())
    }

    pub fn clear(
        &self,
        cmd: &ID3D12GraphicsCommandList,
        heap: &mut D3D12DescriptorHeap<RenderTargetHeap>,
    ) -> error::Result<()> {
        unsafe {
            util::d3d12_resource_transition::<OutlivesFrame, _>(
                cmd,
                &self.resource,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
            );

            let rtv = self.create_render_target_view(heap)?;

            cmd.ClearRenderTargetView(*rtv.descriptor.as_ref(), CLEAR, None);

            util::d3d12_resource_transition::<OutlivesFrame, _>(
                cmd,
                &self.resource,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            );
        }

        Ok(())
    }

    pub(crate) fn create_shader_resource_view(
        &self,
        heap: &mut D3D12DescriptorHeap<CpuStagingHeap>,
        filter: FilterMode,
        wrap_mode: WrapMode,
    ) -> error::Result<InputTexture> {
        let descriptor = heap.allocate_descriptor()?;

        unsafe {
            let srv_desc = D3D12_SHADER_RESOURCE_VIEW_DESC {
                Format: self.format.into(),
                ViewDimension: D3D12_SRV_DIMENSION_TEXTURE2D,
                Shader4ComponentMapping: D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                Anonymous: D3D12_SHADER_RESOURCE_VIEW_DESC_0 {
                    Texture2D: D3D12_TEX2D_SRV {
                        MipLevels: u32::MAX,
                        ..Default::default()
                    },
                },
            };

            self.device.CreateShaderResourceView(
                self.handle.resource(),
                Some(&srv_desc),
                *descriptor.as_ref(),
            );
        }

        Ok(InputTexture::new_owned(
            &self.resource,
            descriptor,
            self.size,
            self.format,
            filter,
            wrap_mode,
        ))
    }

    pub(crate) fn create_render_target_view(
        &self,
        heap: &mut D3D12DescriptorHeap<RenderTargetHeap>,
    ) -> error::Result<D3D12OutputView> {
        unsafe {
            D3D12OutputView::new_from_resource_internal(self.resource.to_ref(), &self.device, heap)
        }
    }

    pub fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        mipmap: bool,
    ) -> error::Result<Size<u32>> {
        let size = source_size.scale_viewport(scaling, *viewport_size, *original_size);
        let format = Self::get_format_support(&self.device, format.into(), mipmap);

        if self.size != size
            || (mipmap && self.max_mipmap == 1)
            || (!mipmap && self.max_mipmap != 1)
            || format != self.format
        {
            let mut new = OwnedImage::new(&self.device, &self.allocator, size, format, mipmap)?;
            std::mem::swap(self, &mut new);
        }
        Ok(size)
    }
}

impl ScaleFramebuffer for OwnedImage {
    type Error = FilterChainError;
    type Context = ();

    fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        should_mipmap: bool,
        _context: &Self::Context,
    ) -> Result<Size<u32>, Self::Error> {
        self.scale(
            scaling,
            format,
            viewport_size,
            source_size,
            original_size,
            should_mipmap,
        )
    }
}

impl Drop for OwnedImage {
    fn drop(&mut self) {
        // let go of the handle
        unsafe {
            ManuallyDrop::drop(&mut self.resource);
        }
        let resource = unsafe { ManuallyDrop::take(&mut self.handle) };
        if let Err(e) = self.allocator.lock().free_resource(resource) {
            println!("librashader-runtime-d3d12: [warn] failed to deallocate owned image buffer memory {e}")
        }
    }
}
