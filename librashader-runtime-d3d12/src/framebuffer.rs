use crate::descriptor_heap::{CpuStagingHeap, D3D12DescriptorHeap, RenderTargetHeap};
use crate::error::{assume_d3d12_init, FilterChainError};
use crate::filter_chain::FrameResiduals;
use crate::texture::{D3D12OutputView, InputTexture};
use crate::util::d3d12_get_closest_format;
use crate::{error, util};
use librashader_common::{FilterMode, ImageFormat, Size, WrapMode};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::{MipmapSize, ScaleFramebuffer, ViewportSize};
use std::mem::ManuallyDrop;
use std::ops::Deref;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, ID3D12Resource, D3D12_BOX,
    D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    D3D12_FEATURE_DATA_FORMAT_SUPPORT, D3D12_FORMAT_SUPPORT1_MIP,
    D3D12_FORMAT_SUPPORT1_RENDER_TARGET, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE,
    D3D12_FORMAT_SUPPORT1_TEXTURE2D, D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD,
    D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE, D3D12_HEAP_FLAG_NONE, D3D12_HEAP_PROPERTIES,
    D3D12_HEAP_TYPE_DEFAULT, D3D12_MEMORY_POOL_UNKNOWN, D3D12_RENDER_TARGET_VIEW_DESC,
    D3D12_RENDER_TARGET_VIEW_DESC_0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_DESC,
    D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST,
    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RTV_DIMENSION_TEXTURE2D,
    D3D12_SHADER_RESOURCE_VIEW_DESC, D3D12_SHADER_RESOURCE_VIEW_DESC_0,
    D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_TEX2D_RTV, D3D12_TEX2D_SRV, D3D12_TEXTURE_COPY_LOCATION,
    D3D12_TEXTURE_COPY_LOCATION_0, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
};
use windows::Win32::Graphics::Dxgi::Common::{DXGI_FORMAT, DXGI_SAMPLE_DESC};

#[derive(Debug, Clone)]
pub(crate) struct OwnedImage {
    pub(crate) handle: ID3D12Resource,
    pub(crate) size: Size<u32>,
    pub(crate) format: DXGI_FORMAT,
    pub(crate) max_mipmap: u16,
    device: ID3D12Device,
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
        let mut resource: Option<ID3D12Resource> = None;
        unsafe {
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
        }
        assume_d3d12_init!(resource, "CreateCommittedResource");

        Ok(OwnedImage {
            handle: resource,
            size,
            format: desc.Format,
            device: device.clone(),
            max_mipmap: miplevels as u16,
        })
    }

    /// SAFETY: self must fit the source image
    /// source must be in D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    pub unsafe fn copy_from(
        &self,
        cmd: &ID3D12GraphicsCommandList,
        input: &InputTexture,
        gc: &mut FrameResiduals,
    ) -> error::Result<()> {
        let barriers = [
            util::d3d12_get_resource_transition_subresource(
                &input.resource,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            ),
            util::d3d12_get_resource_transition_subresource(
                &self.handle,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            ),
        ];

        unsafe {
            cmd.ResourceBarrier(&barriers);

            let dst = D3D12_TEXTURE_COPY_LOCATION {
                pResource: ManuallyDrop::new(Some(self.handle.clone())),
                Type: D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                Anonymous: D3D12_TEXTURE_COPY_LOCATION_0 {
                    SubresourceIndex: 0,
                },
            };

            let src = D3D12_TEXTURE_COPY_LOCATION {
                pResource: ManuallyDrop::new(Some(input.resource.clone())),
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

            gc.dispose_resource(dst.pResource);
            gc.dispose_resource(src.pResource);
        }

        let barriers = [
            util::d3d12_get_resource_transition_subresource(
                &input.resource,
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            ),
            util::d3d12_get_resource_transition_subresource(
                &self.handle,
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
        util::d3d12_resource_transition(
            cmd,
            &self.handle,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
        );

        let rtv = self.create_render_target_view(heap)?;

        unsafe { cmd.ClearRenderTargetView(*rtv.descriptor.as_ref(), CLEAR, None) }

        util::d3d12_resource_transition(
            cmd,
            &self.handle,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        );

        Ok(())
    }

    pub(crate) fn create_shader_resource_view(
        &self,
        heap: &mut D3D12DescriptorHeap<CpuStagingHeap>,
        filter: FilterMode,
        wrap_mode: WrapMode,
    ) -> error::Result<InputTexture> {
        let descriptor = heap.alloc_slot()?;

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
                &self.handle,
                Some(&srv_desc),
                *descriptor.deref().as_ref(),
            );
        }

        Ok(InputTexture::new(
            self.handle.clone(),
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
        let descriptor = heap.alloc_slot()?;

        unsafe {
            let rtv_desc = D3D12_RENDER_TARGET_VIEW_DESC {
                Format: self.format.into(),
                ViewDimension: D3D12_RTV_DIMENSION_TEXTURE2D,
                Anonymous: D3D12_RENDER_TARGET_VIEW_DESC_0 {
                    Texture2D: D3D12_TEX2D_RTV {
                        MipSlice: 0,
                        ..Default::default()
                    },
                },
            };

            self.device.CreateRenderTargetView(
                &self.handle,
                Some(&rtv_desc),
                *descriptor.deref().as_ref(),
            );
        }

        Ok(D3D12OutputView::new(
            descriptor,
            self.size,
            self.format.into(),
        ))
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
            let mut new = OwnedImage::new(&self.device, size, format, mipmap)?;
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
