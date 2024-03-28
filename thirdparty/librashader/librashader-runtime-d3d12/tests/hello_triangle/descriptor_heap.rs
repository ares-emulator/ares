use bitvec::bitvec;
use bitvec::boxed::BitBox;
use bitvec::order::Lsb0;
use std::marker::PhantomData;
use std::ops::Deref;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;

use windows::Win32::Graphics::Direct3D12::{
    ID3D12DescriptorHeap, ID3D12Device, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_DESCRIPTOR_HEAP_DESC,
    D3D12_DESCRIPTOR_HEAP_FLAG_NONE, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    D3D12_DESCRIPTOR_HEAP_TYPE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    D3D12_GPU_DESCRIPTOR_HANDLE,
};

pub trait D3D12HeapType {
    fn get_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC;
}

pub trait D3D12ShaderVisibleHeapType: D3D12HeapType {}

#[derive(Clone)]
pub struct CpuStagingHeap;

impl D3D12HeapType for CpuStagingHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn get_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            NodeMask: 0,
        }
    }
}

pub type D3D12DescriptorHeapSlot<T> = Arc<D3D12DescriptorHeapSlotInner<T>>;

pub struct D3D12DescriptorHeapSlotInner<T> {
    cpu_handle: D3D12_CPU_DESCRIPTOR_HANDLE,
    gpu_handle: Option<D3D12_GPU_DESCRIPTOR_HANDLE>,
    heap: Arc<D3D12DescriptorHeapInner>,
    slot: usize,
    _pd: PhantomData<T>,
}

impl<T> D3D12DescriptorHeapSlotInner<T> {
    /// Get the index of the resource within the heap.
    pub fn index(&self) -> usize {
        self.slot
    }

    /// unsafe because type must match
    pub unsafe fn copy_descriptor(&self, source: D3D12_CPU_DESCRIPTOR_HANDLE) {
        unsafe {
            let heap = &self.heap.deref();

            heap.device
                .CopyDescriptorsSimple(1, self.cpu_handle, source, heap.ty)
        }
    }
}

impl<T> AsRef<D3D12_CPU_DESCRIPTOR_HANDLE> for D3D12DescriptorHeapSlotInner<T> {
    fn as_ref(&self) -> &D3D12_CPU_DESCRIPTOR_HANDLE {
        &self.cpu_handle
    }
}

impl<T: D3D12ShaderVisibleHeapType> AsRef<D3D12_GPU_DESCRIPTOR_HANDLE>
    for D3D12DescriptorHeapSlotInner<T>
{
    fn as_ref(&self) -> &D3D12_GPU_DESCRIPTOR_HANDLE {
        // SAFETY: D3D12ShaderVisibleHeapType must have a GPU handle.
        self.gpu_handle.as_ref().unwrap()
    }
}

impl<T: D3D12ShaderVisibleHeapType> From<&D3D12DescriptorHeap<T>> for ID3D12DescriptorHeap {
    fn from(value: &D3D12DescriptorHeap<T>) -> Self {
        value.0.heap.clone()
    }
}

#[derive(Debug)]
struct D3D12DescriptorHeapInner {
    device: ID3D12Device,
    heap: ID3D12DescriptorHeap,
    ty: D3D12_DESCRIPTOR_HEAP_TYPE,
    cpu_start: D3D12_CPU_DESCRIPTOR_HANDLE,
    gpu_start: Option<D3D12_GPU_DESCRIPTOR_HANDLE>,
    handle_size: usize,
    start: AtomicUsize,
    num_descriptors: usize,
    map: BitBox<AtomicUsize>,
}

pub struct D3D12DescriptorHeap<T>(Arc<D3D12DescriptorHeapInner>, PhantomData<T>);

impl<T: D3D12HeapType> D3D12DescriptorHeap<T> {
    pub fn new(
        device: &ID3D12Device,
        size: usize,
    ) -> Result<D3D12DescriptorHeap<T>, windows::core::Error> {
        let desc = T::get_desc(size);
        unsafe { D3D12DescriptorHeap::new_with_desc(device, desc) }
    }
}

impl<T> D3D12DescriptorHeap<T> {
    /// Gets a cloned handle to the inner heap
    pub fn handle(&self) -> ID3D12DescriptorHeap {
        self.0.heap.clone()
    }

    pub unsafe fn new_with_desc(
        device: &ID3D12Device,
        desc: D3D12_DESCRIPTOR_HEAP_DESC,
    ) -> Result<D3D12DescriptorHeap<T>, windows::core::Error> {
        unsafe {
            let heap: ID3D12DescriptorHeap = device.CreateDescriptorHeap(&desc)?;
            let cpu_start = heap.GetCPUDescriptorHandleForHeapStart();

            let gpu_start = if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE).0 != 0 {
                Some(heap.GetGPUDescriptorHandleForHeapStart())
            } else {
                None
            };

            Ok(D3D12DescriptorHeap(
                Arc::new(D3D12DescriptorHeapInner {
                    device: device.clone(),
                    heap,
                    ty: desc.Type,
                    cpu_start,
                    gpu_start,
                    handle_size: device.GetDescriptorHandleIncrementSize(desc.Type) as usize,
                    start: AtomicUsize::new(0),
                    num_descriptors: desc.NumDescriptors as usize,
                    map: bitvec![AtomicUsize, Lsb0; 0; desc.NumDescriptors as usize]
                        .into_boxed_bitslice(),
                }),
                PhantomData::default(),
            ))
        }
    }

    pub fn alloc_slot(&mut self) -> D3D12DescriptorHeapSlot<T> {
        let mut handle = D3D12_CPU_DESCRIPTOR_HANDLE { ptr: 0 };

        let inner = &self.0;
        let start = inner.start.load(Ordering::Acquire);
        for i in start..inner.num_descriptors {
            if !inner.map[i] {
                inner.map.set_aliased(i, true);
                handle.ptr = inner.cpu_start.ptr + (i * inner.handle_size);
                inner.start.store(i + 1, Ordering::Release);

                let gpu_handle = inner
                    .gpu_start
                    .map(|gpu_start| D3D12_GPU_DESCRIPTOR_HANDLE {
                        ptr: (handle.ptr as u64 - inner.cpu_start.ptr as u64) + gpu_start.ptr,
                    });

                return Arc::new(D3D12DescriptorHeapSlotInner {
                    cpu_handle: handle,
                    slot: i,
                    heap: Arc::clone(&self.0),
                    gpu_handle,
                    _pd: Default::default(),
                });
            }
        }

        panic!("overflow")
    }
}

impl<T> Drop for D3D12DescriptorHeapSlotInner<T> {
    fn drop(&mut self) {
        let inner = &self.heap;
        inner.map.set_aliased(self.slot, false);
        // inner.start > self.slot => inner.start = self.slot
        inner.start.fetch_min(self.slot, Ordering::AcqRel);
    }
}
