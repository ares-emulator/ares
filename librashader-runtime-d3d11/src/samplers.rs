use crate::error::{assume_d3d11_init, Result};
use librashader_common::{FilterMode, WrapMode};
use rustc_hash::FxHashMap;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11Device, ID3D11SamplerState, D3D11_COMPARISON_NEVER, D3D11_FLOAT32_MAX,
    D3D11_SAMPLER_DESC, D3D11_TEXTURE_ADDRESS_MODE,
};
pub struct SamplerSet {
    samplers: FxHashMap<(WrapMode, FilterMode), ID3D11SamplerState>,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(&self, wrap: WrapMode, filter: FilterMode) -> &ID3D11SamplerState {
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter
        unsafe { self.samplers.get(&(wrap, filter)).unwrap_unchecked() }
    }

    pub fn new(device: &ID3D11Device) -> Result<SamplerSet> {
        let mut samplers = FxHashMap::default();
        let wrap_modes = &[
            WrapMode::ClampToBorder,
            WrapMode::ClampToEdge,
            WrapMode::Repeat,
            WrapMode::MirroredRepeat,
        ];
        for wrap_mode in wrap_modes {
            for filter_mode in &[FilterMode::Linear, FilterMode::Nearest] {
                unsafe {
                    let mut sampler = None;
                    device.CreateSamplerState(
                        &D3D11_SAMPLER_DESC {
                            Filter: (*filter_mode).into(),
                            AddressU: D3D11_TEXTURE_ADDRESS_MODE::from(*wrap_mode),
                            AddressV: D3D11_TEXTURE_ADDRESS_MODE::from(*wrap_mode),
                            AddressW: D3D11_TEXTURE_ADDRESS_MODE::from(*wrap_mode),
                            MipLODBias: 0.0,
                            MaxAnisotropy: 1,
                            ComparisonFunc: D3D11_COMPARISON_NEVER,
                            BorderColor: [0.0, 0.0, 0.0, 0.0],
                            MinLOD: -D3D11_FLOAT32_MAX,
                            MaxLOD: D3D11_FLOAT32_MAX,
                        },
                        Some(&mut sampler),
                    )?;

                    assume_d3d11_init!(sampler, "CreateSamplerState");
                    samplers.insert((*wrap_mode, *filter_mode), sampler);
                }
            }
        }

        assert_eq!(samplers.len(), wrap_modes.len() * 2);
        Ok(SamplerSet { samplers })
    }
}
