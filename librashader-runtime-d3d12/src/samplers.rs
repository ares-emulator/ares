use crate::descriptor_heap::{D3D12DescriptorHeap, D3D12DescriptorHeapSlot, SamplerPaletteHeap};
use crate::error;
use librashader_common::{FilterMode, WrapMode};
use rustc_hash::FxHashMap;
use std::ops::Deref;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, D3D12_COMPARISON_FUNC_NEVER, D3D12_FLOAT32_MAX, D3D12_SAMPLER_DESC,
    D3D12_TEXTURE_ADDRESS_MODE,
};

pub struct SamplerSet {
    samplers: FxHashMap<(WrapMode, FilterMode), D3D12DescriptorHeapSlot<SamplerPaletteHeap>>,
    _heap: D3D12DescriptorHeap<SamplerPaletteHeap>,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(
        &self,
        wrap: WrapMode,
        filter: FilterMode,
    ) -> &D3D12DescriptorHeapSlot<SamplerPaletteHeap> {
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter
        unsafe { self.samplers.get(&(wrap, filter)).unwrap_unchecked() }
    }
    pub fn new(device: &ID3D12Device) -> error::Result<SamplerSet> {
        let mut samplers = FxHashMap::default();
        let wrap_modes = &[
            WrapMode::ClampToBorder,
            WrapMode::ClampToEdge,
            WrapMode::Repeat,
            WrapMode::MirroredRepeat,
        ];

        let mut heap = D3D12DescriptorHeap::new(device, 2 * wrap_modes.len())?;

        for wrap_mode in wrap_modes {
            for filter_mode in &[FilterMode::Linear, FilterMode::Nearest] {
                unsafe {
                    let sampler = heap.alloc_slot()?;
                    device.CreateSampler(
                        &D3D12_SAMPLER_DESC {
                            Filter: (*filter_mode).into(),
                            AddressU: D3D12_TEXTURE_ADDRESS_MODE::from(*wrap_mode),
                            AddressV: D3D12_TEXTURE_ADDRESS_MODE::from(*wrap_mode),
                            AddressW: D3D12_TEXTURE_ADDRESS_MODE::from(*wrap_mode),
                            MipLODBias: 0.0,
                            MaxAnisotropy: 1,
                            ComparisonFunc: D3D12_COMPARISON_FUNC_NEVER,
                            BorderColor: [0.0, 0.0, 0.0, 0.0],
                            MinLOD: -D3D12_FLOAT32_MAX,
                            MaxLOD: D3D12_FLOAT32_MAX,
                        },
                        *sampler.deref().as_ref(),
                    );
                    samplers.insert((*wrap_mode, *filter_mode), sampler);
                }
            }
        }

        assert_eq!(samplers.len(), wrap_modes.len() * 2);
        Ok(SamplerSet {
            samplers,
            _heap: heap,
        })
    }
}
