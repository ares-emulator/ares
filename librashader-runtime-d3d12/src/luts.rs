use crate::descriptor_heap::{CpuStagingHeap, D3D12DescriptorHeap};
use crate::error;
use crate::error::assume_d3d12_init;
use crate::filter_chain::FrameResiduals;
use crate::mipmap::MipmapGenContext;
use crate::texture::InputTexture;
use crate::util::{d3d12_get_closest_format, d3d12_resource_transition, d3d12_update_subresources};
use librashader_common::{FilterMode, ImageFormat, WrapMode};
use librashader_runtime::image::Image;
use librashader_runtime::scaling::MipmapSize;
use std::ops::Deref;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, ID3D12Resource, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, D3D12_FEATURE_DATA_FORMAT_SUPPORT,
    D3D12_FORMAT_SUPPORT1_MIP, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE,
    D3D12_FORMAT_SUPPORT1_TEXTURE2D, D3D12_HEAP_FLAG_NONE, D3D12_HEAP_PROPERTIES,
    D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_TYPE_UPLOAD, D3D12_MEMORY_POOL_UNKNOWN,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT, D3D12_RESOURCE_DESC, D3D12_RESOURCE_DIMENSION_BUFFER,
    D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_SHADER_RESOURCE_VIEW_DESC,
    D3D12_SHADER_RESOURCE_VIEW_DESC_0, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_SUBRESOURCE_DATA,
    D3D12_TEX2D_SRV, D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_SAMPLE_DESC;

pub struct LutTexture {
    resource: ID3D12Resource,
    view: InputTexture,
    miplevels: Option<u16>,
    _staging: ID3D12Resource,
}

impl LutTexture {
    pub(crate) fn new(
        device: &ID3D12Device,
        heap: &mut D3D12DescriptorHeap<CpuStagingHeap>,
        cmd: &ID3D12GraphicsCommandList,
        source: &Image,
        filter: FilterMode,
        wrap_mode: WrapMode,
        mipmap: bool,
        gc: &mut FrameResiduals,
    ) -> error::Result<LutTexture> {
        let miplevels = source.size.calculate_miplevels() as u16;
        let mut desc = D3D12_RESOURCE_DESC {
            Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            Alignment: 0,
            Width: source.size.width as u64,
            Height: source.size.height,
            DepthOrArraySize: 1,
            MipLevels: if mipmap { miplevels } else { 1 },
            Format: ImageFormat::R8G8B8A8Unorm.into(),
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                Quality: 0,
            },
            Layout: Default::default(),
            Flags: Default::default(),
        };

        let mut format_support = D3D12_FEATURE_DATA_FORMAT_SUPPORT {
            Format: desc.Format,
            Support1: D3D12_FORMAT_SUPPORT1_TEXTURE2D | D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE,
            ..Default::default()
        };

        if mipmap {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            format_support.Support1 |= D3D12_FORMAT_SUPPORT1_MIP;
        }

        desc.Format = d3d12_get_closest_format(device, format_support);
        let descriptor = heap.alloc_slot()?;

        // create handles on GPU
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
        unsafe {
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

            device.CreateShaderResourceView(
                &resource,
                Some(&srv_desc),
                *descriptor.deref().as_ref(),
            );
        }

        let mut buffer_desc = D3D12_RESOURCE_DESC {
            Dimension: D3D12_RESOURCE_DIMENSION_BUFFER,
            ..Default::default()
        };

        let mut layout = D3D12_PLACED_SUBRESOURCE_FOOTPRINT::default();
        let mut total = 0;
        // texture upload
        unsafe {
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
        }
        let mut upload: Option<ID3D12Resource> = None;

        unsafe {
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
        }
        assume_d3d12_init!(upload, "CreateCommittedResource");

        let subresource = [D3D12_SUBRESOURCE_DATA {
            pData: source.bytes.as_ptr().cast(),
            RowPitch: 4 * source.size.width as isize,
            SlicePitch: (4 * source.size.width * source.size.height) as isize,
        }];

        d3d12_resource_transition(
            cmd,
            &resource,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST,
        );

        d3d12_update_subresources(cmd, &resource, &upload, 0, 0, 1, &subresource, gc)?;

        d3d12_resource_transition(
            cmd,
            &resource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        );

        let view = InputTexture::new(
            resource.clone(),
            descriptor,
            source.size,
            ImageFormat::R8G8B8A8Unorm.into(),
            filter,
            wrap_mode,
        );
        Ok(LutTexture {
            resource,
            _staging: upload,
            view,
            miplevels: if mipmap { Some(miplevels) } else { None },
        })
    }

    pub fn generate_mipmaps(&self, gen_mips: &mut MipmapGenContext) -> error::Result<()> {
        if let Some(miplevels) = self.miplevels {
            gen_mips.generate_mipmaps(
                &self.resource,
                miplevels,
                self.view.size,
                ImageFormat::R8G8B8A8Unorm.into(),
            )?
        }

        Ok(())
    }
}

impl AsRef<InputTexture> for LutTexture {
    fn as_ref(&self) -> &InputTexture {
        &self.view
    }
}
