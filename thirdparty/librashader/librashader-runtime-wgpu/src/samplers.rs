use librashader_common::map::FastHashMap;
use librashader_common::{FilterMode, WrapMode};
use std::sync::Arc;
use wgpu::{AddressMode, Features, Sampler, SamplerBorderColor, SamplerDescriptor};

pub struct SamplerSet {
    // todo: may need to deal with differences in mip filter.
    samplers: FastHashMap<(WrapMode, FilterMode, FilterMode), Arc<Sampler>>,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(&self, wrap: WrapMode, filter: FilterMode, mipmap: FilterMode) -> Arc<Sampler> {
        // eprintln!("{wrap}, {filter}, {mip}");
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter x mipmap
        unsafe {
            Arc::clone(
                &self
                    .samplers
                    .get(&(wrap, filter, mipmap))
                    .unwrap_unchecked(),
            )
        }
    }

    pub fn new(device: &wgpu::Device) -> SamplerSet {
        let mut samplers = FastHashMap::default();
        let wrap_modes = &[
            WrapMode::ClampToBorder,
            WrapMode::ClampToEdge,
            WrapMode::Repeat,
            WrapMode::MirroredRepeat,
        ];

        let has_clamp_to_border = device
            .features()
            .contains(Features::ADDRESS_MODE_CLAMP_TO_BORDER);

        for wrap_mode in wrap_modes {
            for filter_mode in &[FilterMode::Linear, FilterMode::Nearest] {
                for mipmap_filter in &[FilterMode::Linear, FilterMode::Nearest] {
                    let mut address_mode = (*wrap_mode).into();
                    // if the device doesn't have clap to border support,
                    // approximate it with clamp to edge.
                    if !has_clamp_to_border && address_mode == AddressMode::ClampToBorder {
                        address_mode = AddressMode::ClampToEdge;
                    }

                    samplers.insert(
                        (*wrap_mode, *filter_mode, *mipmap_filter),
                        Arc::new(device.create_sampler(&SamplerDescriptor {
                            label: None,
                            address_mode_u: address_mode,
                            address_mode_v: address_mode,
                            address_mode_w: address_mode,
                            mag_filter: (*filter_mode).into(),
                            min_filter: (*filter_mode).into(),
                            mipmap_filter: (*mipmap_filter).into(),
                            lod_min_clamp: 0.0,
                            lod_max_clamp: 1000.0,
                            compare: None,
                            anisotropy_clamp: 1,
                            border_color: Some(SamplerBorderColor::TransparentBlack),
                        })),
                    );
                }
            }
        }

        // assert all samplers were created.
        assert_eq!(samplers.len(), wrap_modes.len() * 2 * 2);
        SamplerSet { samplers }
    }
}
