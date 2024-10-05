use crate::error;
use gpu_allocator::d3d12::{
    Allocator, Resource, ResourceCategory, ResourceCreateDesc, ResourceStateOrBarrierLayout,
    ResourceType,
};
use gpu_allocator::MemoryLocation;
use parking_lot::Mutex;
use std::ffi::c_void;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut, Range};
use std::ptr::NonNull;
use std::sync::Arc;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12GraphicsCommandList, ID3D12Resource, D3D12_RANGE, D3D12_RESOURCE_DESC,
    D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_STATE_GENERIC_READ,
    D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_SAMPLE_DESC;

pub struct D3D12Buffer {
    resource: ManuallyDrop<Resource>,
    allocator: Arc<Mutex<Allocator>>,
    size: usize,
}

pub struct D3D12BufferMapHandle<'a> {
    pub slice: &'a mut [u8],
    pub handle: &'a ID3D12Resource,
}

impl<'a> Drop for D3D12BufferMapHandle<'a> {
    fn drop(&mut self) {
        unsafe { self.handle.Unmap(0, None) }
    }
}

impl Drop for D3D12Buffer {
    fn drop(&mut self) {
        let resource = unsafe { ManuallyDrop::take(&mut self.resource) };
        if let Err(e) = self.allocator.lock().free_resource(resource) {
            println!("librashader-runtime-d3d12: [warn] failed to deallocate buffer memory {e}")
        }
    }
}

impl D3D12Buffer {
    pub fn new(allocator: &Arc<Mutex<Allocator>>, size: usize) -> error::Result<D3D12Buffer> {
        let resource = allocator.lock().create_resource(&ResourceCreateDesc {
            name: "d3d12buffer",
            memory_location: MemoryLocation::CpuToGpu,
            resource_category: ResourceCategory::Buffer,
            resource_desc: &D3D12_RESOURCE_DESC {
                Dimension: D3D12_RESOURCE_DIMENSION_BUFFER,
                Width: size as u64,
                Height: 1,
                DepthOrArraySize: 1,
                MipLevels: 1,
                Layout: D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                SampleDesc: DXGI_SAMPLE_DESC {
                    Count: 1,
                    Quality: 0,
                },
                ..Default::default()
            },
            castable_formats: &[],
            clear_value: None,
            initial_state_or_layout: ResourceStateOrBarrierLayout::ResourceState(
                D3D12_RESOURCE_STATE_GENERIC_READ,
            ),
            resource_type: &ResourceType::Placed,
        })?;

        Ok(Self {
            resource: ManuallyDrop::new(resource),
            size,
            allocator: Arc::clone(&allocator),
        })
    }

    pub fn gpu_address(&self) -> u64 {
        unsafe { self.resource.resource().GetGPUVirtualAddress() }
    }

    pub fn map(&mut self, range: Option<Range<usize>>) -> error::Result<D3D12BufferMapHandle> {
        let (range, size) = range.map_or((D3D12_RANGE { Begin: 0, End: 0 }, self.size), |range| {
            (
                D3D12_RANGE {
                    Begin: range.start,
                    End: range.end,
                },
                range.end - range.start,
            )
        });

        unsafe {
            let mut ptr = std::ptr::null_mut();
            self.resource
                .resource()
                .Map(0, Some(&range), Some(&mut ptr))?;
            let slice = std::slice::from_raw_parts_mut(ptr.cast(), size);
            Ok(D3D12BufferMapHandle {
                handle: &self.resource.resource(),
                slice,
            })
        }
    }
}

/// SAFETY: Creating the pointer should be safe in multithreaded contexts.
///
/// Mutation is guarded by DerefMut<Target=[u8]>, so exclusive access is guaranteed.
/// We do not ever leak the pointer to C.
unsafe impl Send for RawD3D12Buffer {}
unsafe impl Sync for RawD3D12Buffer {}
pub struct RawD3D12Buffer {
    buffer: ManuallyDrop<D3D12Buffer>,
    ptr: NonNull<c_void>,
}

impl RawD3D12Buffer {
    pub fn new(buffer: D3D12Buffer) -> error::Result<Self> {
        let buffer = ManuallyDrop::new(buffer);
        let range = D3D12_RANGE { Begin: 0, End: 0 };
        let mut ptr = std::ptr::null_mut();
        unsafe {
            buffer
                .resource
                .resource()
                .Map(0, Some(&range), Some(&mut ptr))?;
        }

        // panic-safety: If Map returns Ok then ptr is not null
        assert!(!ptr.is_null());

        let ptr = unsafe { NonNull::new_unchecked(ptr) };

        Ok(RawD3D12Buffer { buffer, ptr })
    }

    pub fn bind_cbv(&self, index: u32, cmd: &ID3D12GraphicsCommandList) {
        unsafe { cmd.SetGraphicsRootConstantBufferView(index, self.buffer.gpu_address()) }
    }
}

impl Drop for RawD3D12Buffer {
    fn drop(&mut self) {
        unsafe {
            self.buffer.resource.resource().Unmap(0, None);
            ManuallyDrop::drop(&mut self.buffer);
        }
    }
}

impl Deref for RawD3D12Buffer {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        unsafe { std::slice::from_raw_parts(self.ptr.as_ptr().cast(), self.buffer.size) }
    }
}

impl DerefMut for RawD3D12Buffer {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { std::slice::from_raw_parts_mut(self.ptr.as_ptr().cast(), self.buffer.size) }
    }
}
