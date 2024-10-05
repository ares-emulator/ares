use anyhow::anyhow;
use std::mem::ManuallyDrop;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, ID3D12Resource, D3D12_MEMCPY_DEST,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT, D3D12_RESOURCE_BARRIER, D3D12_RESOURCE_BARRIER_0,
    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE,
    D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_STATES,
    D3D12_RESOURCE_TRANSITION_BARRIER, D3D12_SUBRESOURCE_DATA, D3D12_TEXTURE_COPY_LOCATION,
    D3D12_TEXTURE_COPY_LOCATION_0, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
    D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
};

#[inline(always)]
pub fn d3d12_resource_transition(
    cmd: &ID3D12GraphicsCommandList,
    resource: &ID3D12Resource,
    before: D3D12_RESOURCE_STATES,
    after: D3D12_RESOURCE_STATES,
) {
    d3d12_resource_transition_subresource(
        cmd,
        resource,
        before,
        after,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
    );
}

pub fn d3d12_get_resource_transition_subresource(
    resource: &ID3D12Resource,
    before: D3D12_RESOURCE_STATES,
    after: D3D12_RESOURCE_STATES,
    subresource: u32,
) -> D3D12_RESOURCE_BARRIER {
    D3D12_RESOURCE_BARRIER {
        Type: D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        Flags: D3D12_RESOURCE_BARRIER_FLAG_NONE,
        Anonymous: D3D12_RESOURCE_BARRIER_0 {
            Transition: ManuallyDrop::new(D3D12_RESOURCE_TRANSITION_BARRIER {
                pResource: ManuallyDrop::new(Some(resource.clone())),
                Subresource: subresource,
                StateBefore: before,
                StateAfter: after,
            }),
        },
    }
}

#[inline(always)]
pub fn d3d12_resource_transition_subresource(
    cmd: &ID3D12GraphicsCommandList,
    resource: &ID3D12Resource,
    before: D3D12_RESOURCE_STATES,
    after: D3D12_RESOURCE_STATES,
    subresource: u32,
) {
    let barrier = [d3d12_get_resource_transition_subresource(
        resource,
        before,
        after,
        subresource,
    )];
    unsafe { cmd.ResourceBarrier(&barrier) }
}

pub(crate) fn d3d12_update_subresources(
    cmd: &ID3D12GraphicsCommandList,
    destination_resource: &ID3D12Resource,
    intermediate_resource: &ID3D12Resource,
    intermediate_offset: u64,
    first_subresouce: u32,
    num_subresources: u32,
    source: &[D3D12_SUBRESOURCE_DATA],
) -> anyhow::Result<u64> {
    // let allocation_size = std::mem::size_of::<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>()
    //     + std::mem::size_of::<u32>()
    //     + std::mem::size_of::<u64>() * num_subresources;

    unsafe {
        let destination_desc = destination_resource.GetDesc();
        let mut device: Option<ID3D12Device> = None;
        destination_resource.GetDevice(&mut device)?;
        let device = device.ok_or_else(|| anyhow!("Unable to get device"))?;

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

        update_subresources(
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
        )
    }
}

#[allow(clippy::too_many_arguments)]
fn update_subresources(
    cmd: &ID3D12GraphicsCommandList,
    destination_resource: &ID3D12Resource,
    intermediate_resource: &ID3D12Resource,
    first_subresouce: u32,
    num_subresources: u32,
    required_size: u64,
    layouts: &[D3D12_PLACED_SUBRESOURCE_FOOTPRINT],
    num_rows: &[u32],
    row_sizes_in_bytes: &[u64],
    source_data: &[D3D12_SUBRESOURCE_DATA],
) -> anyhow::Result<u64> {
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
                destination_resource,
                0,
                intermediate_resource,
                layouts[0].Offset,
                layouts[0].Footprint.Width as u64,
            );
        } else {
            for i in 0..num_subresources as usize {
                let dest_location = D3D12_TEXTURE_COPY_LOCATION {
                    pResource: ManuallyDrop::new(Some(destination_resource.clone())),
                    Type: D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                    Anonymous: D3D12_TEXTURE_COPY_LOCATION_0 {
                        SubresourceIndex: i as u32 + first_subresouce,
                    },
                };

                let source_location = D3D12_TEXTURE_COPY_LOCATION {
                    pResource: ManuallyDrop::new(Some(intermediate_resource.clone())),
                    Type: D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                    Anonymous: D3D12_TEXTURE_COPY_LOCATION_0 {
                        PlacedFootprint: layouts[i],
                    },
                };

                cmd.CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, None);
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
