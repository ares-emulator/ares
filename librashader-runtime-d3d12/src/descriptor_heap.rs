use windows::Win32::Graphics::Direct3D12::{
    D3D12_DESCRIPTOR_HEAP_DESC, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
};

use d3d12_descriptor_heap::{D3D12DescriptorHeapType, D3D12ShaderVisibleDescriptorHeapType};

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

impl D3D12DescriptorHeapType for SamplerPaletteHeap {
    // sampler palettes just get set directly
    fn create_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            NodeMask: 0,
        }
    }
}

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

impl D3D12DescriptorHeapType for RenderTargetHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn create_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            NodeMask: 0,
        }
    }
}

unsafe impl D3D12ShaderVisibleDescriptorHeapType for ResourceWorkHeap {}
impl D3D12DescriptorHeapType for ResourceWorkHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn create_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            NodeMask: 0,
        }
    }
}

unsafe impl D3D12ShaderVisibleDescriptorHeapType for SamplerWorkHeap {}
impl D3D12DescriptorHeapType for SamplerWorkHeap {
    // Lut texture heaps are CPU only and get bound to the descriptor heap of the shader.
    fn create_desc(size: usize) -> D3D12_DESCRIPTOR_HEAP_DESC {
        D3D12_DESCRIPTOR_HEAP_DESC {
            Type: D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            NumDescriptors: size as u32,
            Flags: D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            NodeMask: 0,
        }
    }
}
