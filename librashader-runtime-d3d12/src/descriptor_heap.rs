use crate::error;
use bitvec::bitvec;
use bitvec::boxed::BitBox;
use parking_lot::RwLock;
use std::marker::PhantomData;
use std::ops::Deref;
use std::sync::Arc;

use crate::error::FilterChainError;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12DescriptorHeap, ID3D12Device, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_DESCRIPTOR_HEAP_DESC,
    D3D12_DESCRIPTOR_HEAP_FLAG_NONE, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    D3D12_DESCRIPTOR_HEAP_TYPE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
    D3D12_GPU_DESCRIPTOR_HANDLE,
};

pub trait D3D12HeapType {
    fn get_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC;
}

pub trait D3D12ShaderVisibleHeapType: D3D12HeapType {}
#[derive(Clone)]
pub struct SamplerPaletteHeap;

#[derive(Clone)]
pub struct CpuStagingHeap;

#[derive(Clone)]
pub struct RenderTargetHeap;

#[derive(Clone)]
pub struct ResourceWorkHeap;

#[derive(Clone)]
pub struct SamplerWorkHeap;

impl D3D12HeapType for SamplerPaletteHeap {
    // sampler palettes just get set directly
    fn get_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            NodeMask: 0,
        }
    }
}

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

impl D3D12HeapType for RenderTargetHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn get_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            NodeMask: 0,
        }
    }
}

impl D3D12ShaderVisibleHeapType for ResourceWorkHeap {}
impl D3D12HeapType for ResourceWorkHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn get_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            NodeMask: 0,
        }
    }
}

impl D3D12ShaderVisibleHeapType for SamplerWorkHeap {}
impl D3D12HeapType for SamplerWorkHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn get_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            NodeMask: 0,
        }
    }
}

pub type D3D12DescriptorHeapSlot<T> = Arc<D3D12DescriptorHeapSlotInner<T>>;

pub struct D3D12DescriptorHeapSlotInner<T> {
    cpu_handle: D3D12_CPU_DESCRIPTOR_HANDLE,
    gpu_handle: Option<D3D12_GPU_DESCRIPTOR_HANDLE>,
    heap: Arc<RwLock<D3D12DescriptorHeapInner>>,
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
            let heap = self.heap.deref().read();

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
        value.0.read().heap.clone()
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
    start: usize,
    num_descriptors: usize,
    map: BitBox,
}

pub struct D3D12DescriptorHeap<T>(Arc<RwLock<D3D12DescriptorHeapInner>>, PhantomData<T>);

impl<T: D3D12HeapType> D3D12DescriptorHeap<T> {
    pub fn new(device: &ID3D12Device, size: usize) -> error::Result<D3D12DescriptorHeap<T>> {
        let desc = T::get_desc(size);
        unsafe { D3D12DescriptorHeap::new_with_desc(device, desc) }
    }
}

impl<T> D3D12DescriptorHeap<T> {
    /// Gets a cloned handle to the inner heap
    pub fn handle(&self) -> ID3D12DescriptorHeap {
        let inner = self.0.read();
        inner.heap.clone()
    }

    pub unsafe fn new_with_desc(
        device: &ID3D12Device,
        desc: D3D12_DESCRIPTOR_HEAP_DESC,
    ) -> error::Result<D3D12DescriptorHeap<T>> {
        unsafe {
            let heap: ID3D12DescriptorHeap = device.CreateDescriptorHeap(&desc)?;
            let cpu_start = heap.GetCPUDescriptorHandleForHeapStart();

            let gpu_start = if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE).0 != 0 {
                Some(heap.GetGPUDescriptorHandleForHeapStart())
            } else {
                None
            };

            Ok(D3D12DescriptorHeap(
                Arc::new(RwLock::new(D3D12DescriptorHeapInner {
                    device: device.clone(),
                    heap,
                    ty: desc.Type,
                    cpu_start,
                    gpu_start,
                    handle_size: device.GetDescriptorHandleIncrementSize(desc.Type) as usize,
                    start: 0,
                    num_descriptors: desc.NumDescriptors as usize,
                    map: bitvec![0; desc.NumDescriptors as usize].into_boxed_bitslice(),
                })),
                PhantomData::default(),
            ))
        }
    }

    /// suballocates this heap into equally sized chunks.
    /// if there aren't enough descriptors, throws an error.
    ///
    /// it is UB (programmer error) to call this if the descriptor heap already has
    /// descriptors allocated for it.
    ///
    /// size must also divide equally into the size of the heap.
    pub unsafe fn suballocate(
        self,
        size: usize,
        reserved: usize,
    ) -> (
        Vec<D3D12DescriptorHeap<T>>,
        Option<D3D12DescriptorHeap<T>>,
        ID3D12DescriptorHeap,
    ) {
        // has to be called right after creation.
        assert_eq!(
            Arc::strong_count(&self.0),
            1,
            "D3D12DescriptorHeap::suballocate can only be callled immediately after creation."
        );

        let inner = Arc::try_unwrap(self.0)
            .expect("[d3d12] undefined behaviour to suballocate a descriptor heap with live descriptors.")
            .into_inner();

        let num_descriptors = inner.num_descriptors - reserved;

        // number of suballocated heaps
        let num_heaps = num_descriptors / size;
        let remainder = num_descriptors % size;

        assert_eq!(
            remainder, 0,
            "D3D12DescriptorHeap::suballocate \
            must be called with a size that equally divides the number of descriptors"
        );

        let mut heaps = Vec::new();

        let mut start = 0;
        let root_cpu_ptr = inner.cpu_start.ptr;
        let root_gpu_ptr = inner.gpu_start.map(|p| p.ptr);

        for _ in 0..num_heaps {
            let new_cpu_start = root_cpu_ptr + (start * inner.handle_size);
            let new_gpu_start = root_gpu_ptr.map(|r| D3D12_GPU_DESCRIPTOR_HANDLE {
                ptr: r + (start as u64 * inner.handle_size as u64),
            });

            heaps.push(D3D12DescriptorHeapInner {
                device: inner.device.clone(),
                heap: inner.heap.clone(),
                ty: inner.ty,
                cpu_start: D3D12_CPU_DESCRIPTOR_HANDLE { ptr: new_cpu_start },
                gpu_start: new_gpu_start,
                handle_size: inner.handle_size,
                start: 0,
                num_descriptors: size,
                map: bitvec![0; size].into_boxed_bitslice(),
            });

            start += size;
        }

        let mut reserved_heap = None;
        if reserved != 0 {
            assert_eq!(
                reserved,
                inner.num_descriptors - start,
                "The input heap could not fit the number of requested reserved descriptors."
            );
            let new_cpu_start = root_cpu_ptr + (start * inner.handle_size);
            let new_gpu_start = root_gpu_ptr.map(|r| D3D12_GPU_DESCRIPTOR_HANDLE {
                ptr: r + (start as u64 * inner.handle_size as u64),
            });

            reserved_heap = Some(D3D12DescriptorHeapInner {
                device: inner.device.clone(),
                heap: inner.heap.clone(),
                ty: inner.ty,
                cpu_start: D3D12_CPU_DESCRIPTOR_HANDLE { ptr: new_cpu_start },
                gpu_start: new_gpu_start,
                handle_size: inner.handle_size,
                start: 0,
                num_descriptors: reserved,
                map: bitvec![0; reserved].into_boxed_bitslice(),
            });
        }

        (
            heaps
                .into_iter()
                .map(|inner| {
                    D3D12DescriptorHeap(Arc::new(RwLock::new(inner)), PhantomData::default())
                })
                .collect(),
            reserved_heap.map(|inner| {
                D3D12DescriptorHeap(Arc::new(RwLock::new(inner)), PhantomData::default())
            }),
            inner.heap,
        )
    }

    pub fn alloc_slot(&mut self) -> error::Result<D3D12DescriptorHeapSlot<T>> {
        let mut handle = D3D12_CPU_DESCRIPTOR_HANDLE { ptr: 0 };

        let mut inner = self.0.write();
        for i in inner.start..inner.num_descriptors {
            if !inner.map[i] {
                inner.map.set(i, true);
                handle.ptr = inner.cpu_start.ptr + (i * inner.handle_size);
                inner.start = i + 1;

                let gpu_handle = inner
                    .gpu_start
                    .map(|gpu_start| D3D12_GPU_DESCRIPTOR_HANDLE {
                        ptr: (handle.ptr as u64 - inner.cpu_start.ptr as u64) + gpu_start.ptr,
                    });

                return Ok(Arc::new(D3D12DescriptorHeapSlotInner {
                    cpu_handle: handle,
                    slot: i,
                    heap: Arc::clone(&self.0),
                    gpu_handle,
                    _pd: Default::default(),
                }));
            }
        }

        Err(FilterChainError::DescriptorHeapOverflow(
            inner.num_descriptors,
        ))
    }

    pub fn alloc_range<const NUM_DESC: usize>(
        &mut self,
    ) -> error::Result<[D3D12DescriptorHeapSlot<T>; NUM_DESC]> {
        let dest = array_init::try_array_init(|_| self.alloc_slot())?;
        Ok(dest)
    }
}

impl<T> Drop for D3D12DescriptorHeapSlotInner<T> {
    fn drop(&mut self) {
        let mut inner = self.heap.write();
        inner.map.set(self.slot, false);
        if inner.start > self.slot {
            inner.start = self.slot
        }
    }
}
