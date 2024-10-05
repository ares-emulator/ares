use crate::error;
use ash::vk;
use librashader_common::map::FastHashMap;
use librashader_common::{FilterMode, WrapMode};
use std::sync::Arc;

pub struct VulkanSampler {
    pub handle: vk::Sampler,
    device: Arc<ash::Device>,
}

impl VulkanSampler {
    pub fn new(
        device: &Arc<ash::Device>,
        wrap: WrapMode,
        filter: FilterMode,
        mipmap: FilterMode,
    ) -> error::Result<VulkanSampler> {
        let create_info = vk::SamplerCreateInfo::default()
            .mip_lod_bias(0.0)
            .max_anisotropy(1.0)
            .compare_enable(false)
            .min_lod(0.0)
            .max_lod(vk::LOD_CLAMP_NONE)
            .unnormalized_coordinates(false)
            .border_color(vk::BorderColor::FLOAT_TRANSPARENT_BLACK)
            .mag_filter(filter.into())
            .min_filter(filter.into())
            .mipmap_mode(mipmap.into())
            .address_mode_u(wrap.into())
            .address_mode_v(wrap.into())
            .address_mode_w(wrap.into());

        let sampler = unsafe { device.create_sampler(&create_info, None)? };

        Ok(VulkanSampler {
            handle: sampler,
            device: device.clone(),
        })
    }
}

impl Drop for VulkanSampler {
    fn drop(&mut self) {
        if self.handle != vk::Sampler::null() {
            unsafe {
                self.device.destroy_sampler(self.handle, None);
            }
        }
    }
}

pub struct SamplerSet {
    // todo: may need to deal with differences in mip filter.
    samplers: FastHashMap<(WrapMode, FilterMode, FilterMode), VulkanSampler>,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(&self, wrap: WrapMode, filter: FilterMode, mipmap: FilterMode) -> &VulkanSampler {
        // eprintln!("{wrap}, {filter}, {mip}");
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter x mipmap
        unsafe {
            self.samplers
                .get(&(wrap, filter, mipmap))
                .unwrap_unchecked()
        }
    }

    pub fn new(device: &Arc<ash::Device>) -> error::Result<SamplerSet> {
        let mut samplers = FastHashMap::default();
        let wrap_modes = &[
            WrapMode::ClampToBorder,
            WrapMode::ClampToEdge,
            WrapMode::Repeat,
            WrapMode::MirroredRepeat,
        ];
        for wrap_mode in wrap_modes {
            for filter_mode in &[FilterMode::Linear, FilterMode::Nearest] {
                for mipmap_filter in &[FilterMode::Linear, FilterMode::Nearest] {
                    samplers.insert(
                        (*wrap_mode, *filter_mode, *mipmap_filter),
                        VulkanSampler::new(device, *wrap_mode, *filter_mode, *mipmap_filter)?,
                    );
                }
            }
        }

        // assert all samplers were created.
        assert_eq!(samplers.len(), wrap_modes.len() * 2 * 2);
        Ok(SamplerSet { samplers })
    }
}
