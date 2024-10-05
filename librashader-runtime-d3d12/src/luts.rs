use crate::descriptor_heap::CpuStagingHeap;
use crate::error;
use crate::error::assume_d3d12_init;
use crate::filter_chain::FrameResiduals;
use crate::mipmap::MipmapGenContext;
use crate::resource::OutlivesFrame;
use crate::resource::ResourceHandleStrategy;
use crate::texture::InputTexture;
use crate::util::{d3d12_get_closest_format, d3d12_resource_transition};
use d3d12_descriptor_heap::D3D12DescriptorHeap;
use gpu_allocator::d3d12::{
    Allocator, Resource, ResourceCategory, ResourceCreateDesc, ResourceStateOrBarrierLayout,
    ResourceType,
};
use gpu_allocator::MemoryLocation;
use librashader_common::{FilterMode, ImageFormat, WrapMode};
use librashader_runtime::image::Image;
use librashader_runtime::scaling::MipmapSize;
use parking_lot::Mutex;
use std::mem::ManuallyDrop;
use std::ops::Deref;
use std::sync::Arc;
use std::u64;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, ID3D12Resource,
    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, D3D12_FEATURE_DATA_FORMAT_SUPPORT,
    D3D12_FORMAT_SUPPORT1_MIP, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE,
    D3D12_FORMAT_SUPPORT1_TEXTURE2D, D3D12_MEMCPY_DEST, D3D12_PLACED_SUBRESOURCE_FOOTPRINT,
    D3D12_RESOURCE_DESC, D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST,
    D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_SHADER_RESOURCE_VIEW_DESC, D3D12_SHADER_RESOURCE_VIEW_DESC_0,
    D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_SUBRESOURCE_DATA, D3D12_TEX2D_SRV,
    D3D12_TEXTURE_COPY_LOCATION, D3D12_TEXTURE_COPY_LOCATION_0,
    D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_SAMPLE_DESC;

pub struct LutTexture {
    allocator_resource: ManuallyDrop<Resource>,
    resource: ManuallyDrop<ID3D12Resource>,
    view: InputTexture,
    miplevels: Option<u16>,
    // Staging heap needs to be kept alive until the command list is submitted, which is
    // really annoying. We could probably do better but it's safer to keep it around.
    allocator_staging: ManuallyDrop<Resource>,
    staging: ManuallyDrop<ID3D12Resource>,
    allocator: Arc<Mutex<Allocator>>,
}

impl LutTexture {
    pub(crate) fn new(
        device: &ID3D12Device,
        allocator: &Arc<Mutex<Allocator>>,
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
        let descriptor = heap.allocate_descriptor()?;

        // create handles on GPU
        let allocator_resource = allocator.lock().create_resource(&ResourceCreateDesc {
            name: "lut alloc",
            memory_location: MemoryLocation::GpuOnly,
            resource_category: ResourceCategory::OtherTexture,
            resource_desc: &desc,
            castable_formats: &[],
            clear_value: None,
            initial_state_or_layout: ResourceStateOrBarrierLayout::ResourceState(
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            ),
            resource_type: &ResourceType::Placed,
        })?;

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
                allocator_resource.resource(),
                Some(&srv_desc),
                *descriptor.as_ref(),
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

        let allocator_upload = allocator.lock().create_resource(&ResourceCreateDesc {
            name: "lut staging",
            memory_location: MemoryLocation::CpuToGpu,
            resource_category: ResourceCategory::Buffer,
            resource_desc: &buffer_desc,
            castable_formats: &[],
            clear_value: None,
            initial_state_or_layout: ResourceStateOrBarrierLayout::ResourceState(
                D3D12_RESOURCE_STATE_GENERIC_READ,
            ),
            resource_type: &ResourceType::Placed,
        })?;

        let subresource = [D3D12_SUBRESOURCE_DATA {
            pData: source.bytes.as_ptr().cast(),
            RowPitch: 4 * source.size.width as isize,
            SlicePitch: (4 * source.size.width * source.size.height) as isize,
        }];

        let resource = ManuallyDrop::new(allocator_resource.resource().clone());
        let upload = ManuallyDrop::new(allocator_upload.resource().clone());

        d3d12_resource_transition::<OutlivesFrame, _>(
            cmd,
            &resource,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST,
        );

        d3d12_update_subresources::<OutlivesFrame>(
            cmd,
            &resource,
            &upload,
            0,
            0,
            1,
            &subresource,
            gc,
        )?;

        d3d12_resource_transition::<OutlivesFrame, _>(
            cmd,
            &resource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        );

        let view = InputTexture::new_owned(
            &resource,
            descriptor,
            source.size,
            ImageFormat::R8G8B8A8Unorm.into(),
            filter,
            wrap_mode,
        );

        Ok(LutTexture {
            allocator_resource: ManuallyDrop::new(allocator_resource),
            allocator_staging: ManuallyDrop::new(allocator_upload),
            view,
            miplevels: if mipmap { Some(miplevels) } else { None },
            allocator: Arc::clone(&allocator),
            resource,
            staging: upload,
        })
    }

    pub fn generate_mipmaps(&self, gen_mips: &mut MipmapGenContext) -> error::Result<()> {
        if let Some(miplevels) = self.miplevels {
            gen_mips.generate_mipmaps::<OutlivesFrame, _>(
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

impl Drop for LutTexture {
    fn drop(&mut self) {
        // drop view handles
        unsafe {
            ManuallyDrop::drop(&mut self.resource);
            ManuallyDrop::drop(&mut self.staging)
        };

        // deallocate
        let resource = unsafe { ManuallyDrop::take(&mut self.allocator_resource) };
        if let Err(e) = self.allocator.lock().free_resource(resource) {
            println!("librashader-runtime-d3d12: [warn] failed to deallocate lut buffer memory {e}")
        }

        let staging = unsafe { ManuallyDrop::take(&mut self.allocator_staging) };
        if let Err(e) = self.allocator.lock().free_resource(staging) {
            println!("librashader-runtime-d3d12: [warn] failed to deallocate lut staging buffer memory {e}")
        }
    }
}

fn d3d12_update_subresources<S: ResourceHandleStrategy<ManuallyDrop<ID3D12Resource>>>(
    cmd: &ID3D12GraphicsCommandList,
    destination_resource: &ManuallyDrop<ID3D12Resource>,
    intermediate_resource: &ManuallyDrop<ID3D12Resource>,
    intermediate_offset: u64,
    first_subresouce: u32,
    num_subresources: u32,
    source: &[D3D12_SUBRESOURCE_DATA],
    gc: &mut FrameResiduals,
) -> error::Result<u64> {
    // let allocation_size = std::mem::size_of::<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>()
    //     + std::mem::size_of::<u32>()
    //     + std::mem::size_of::<u64>() * num_subresources;

    unsafe {
        let destination_desc = destination_resource.GetDesc();
        let mut device: Option<ID3D12Device> = None;
        destination_resource.GetDevice(&mut device)?;
        assume_d3d12_init!(device, "GetDevice");

        let mut layouts =
            vec![D3D12_PLACED_SUBRESOURCE_FOOTPRINT::default(); num_subresources as usize];
        let mut num_rows = vec![0; num_subresources as usize];
        let mut row_sizes_in_bytes = vec![0; num_subresources as usize];
        let mut required_size = 0;

        // texture upload
        device.GetCopyableFootprints(
            &destination_desc,
            first_subresouce,
            num_subresources,
            intermediate_offset,
            Some(layouts.as_mut_ptr()),
            Some(num_rows.as_mut_ptr()),
            Some(row_sizes_in_bytes.as_mut_ptr()),
            Some(&mut required_size),
        );

        update_subresources::<S>(
            cmd,
            destination_resource,
            intermediate_resource,
            first_subresouce,
            num_subresources,
            required_size,
            &layouts,
            &num_rows,
            &row_sizes_in_bytes,
            source,
            gc,
        )
    }
}

#[allow(clippy::too_many_arguments)]
fn update_subresources<S: ResourceHandleStrategy<ManuallyDrop<ID3D12Resource>>>(
    cmd: &ID3D12GraphicsCommandList,
    destination_resource: &ManuallyDrop<ID3D12Resource>,
    intermediate_resource: &ManuallyDrop<ID3D12Resource>,
    first_subresouce: u32,
    num_subresources: u32,
    required_size: u64,
    layouts: &[D3D12_PLACED_SUBRESOURCE_FOOTPRINT],
    num_rows: &[u32],
    row_sizes_in_bytes: &[u64],
    source_data: &[D3D12_SUBRESOURCE_DATA],
    gc: &mut FrameResiduals,
) -> error::Result<u64> {
    // ToDo: implement validation as in the original function

    unsafe {
        let mut data = std::ptr::null_mut();
        intermediate_resource.Map(0, None, Some(&mut data))?;

        for i in 0..num_subresources as usize {
            let dest_data = D3D12_MEMCPY_DEST {
                pData: data.offset(layouts[i].Offset as isize) as *mut std::ffi::c_void,
                RowPitch: layouts[i].Footprint.RowPitch as usize,
                SlicePitch: ((layouts[i].Footprint.RowPitch) * num_rows[i]) as usize,
            };

            memcpy_subresource(
                &dest_data,
                &source_data[i],
                row_sizes_in_bytes[i],
                num_rows[i],
                layouts[i].Footprint.Depth,
            );
        }
        intermediate_resource.Unmap(0, None);
    }

    unsafe {
        let destination_desc = destination_resource.GetDesc();
        if destination_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER {
            cmd.CopyBufferRegion(
                destination_resource.deref(),
                0,
                intermediate_resource.deref(),
                layouts[0].Offset,
                layouts[0].Footprint.Width as u64,
            );
        } else {
            for i in 0..num_subresources as usize {
                let dest_location = D3D12_TEXTURE_COPY_LOCATION {
                    pResource: S::obtain(&destination_resource),
                    Type: D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                    Anonymous: D3D12_TEXTURE_COPY_LOCATION_0 {
                        SubresourceIndex: i as u32 + first_subresouce,
                    },
                };

                let source_location = D3D12_TEXTURE_COPY_LOCATION {
                    pResource: S::obtain(&intermediate_resource),
                    Type: D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                    Anonymous: D3D12_TEXTURE_COPY_LOCATION_0 {
                        PlacedFootprint: layouts[i],
                    },
                };

                cmd.CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, None);

                S::cleanup_handler(|| gc.dispose_resource(dest_location.pResource));
                S::cleanup_handler(|| gc.dispose_resource(source_location.pResource));
            }
        }
        Ok(required_size)
    }
}

// this function should not leak to the public API, so
// there is no point in using struct wrappers
unsafe fn memcpy_subresource(
    dest: &D3D12_MEMCPY_DEST,
    src: &D3D12_SUBRESOURCE_DATA,
    row_sizes_in_bytes: u64,
    num_rows: u32,
    num_slices: u32,
) {
    unsafe {
        for z in 0..num_slices as usize {
            let dest_slice = dest.pData.add(dest.SlicePitch * z);
            let src_slice = src.pData.offset(src.SlicePitch * z as isize);

            for y in 0..num_rows as usize {
                std::ptr::copy_nonoverlapping(
                    src_slice.offset(src.RowPitch * y as isize),
                    dest_slice.add(dest.RowPitch * y),
                    row_sizes_in_bytes as usize,
                );
            }
        }
    }
}
