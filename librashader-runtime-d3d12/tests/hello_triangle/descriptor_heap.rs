use d3d12_descriptor_heap::D3D12DescriptorHeapType;
use windows::Win32::Graphics::Direct3D12::{
    D3D12_DESCRIPTOR_HEAP_DESC, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
};

#[derive(Clone)]
pub struct CpuStagingHeap;

impl D3D12DescriptorHeapType for CpuStagingHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn create_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            NodeMask: 0,
        }
    }
}
