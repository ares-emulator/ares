use icrate::Metal::{
    MTLCompareFunctionNever, MTLDevice, MTLSamplerAddressMode,
    MTLSamplerBorderColorTransparentBlack, MTLSamplerDescriptor, MTLSamplerMinMagFilter,
    MTLSamplerState,
};
use librashader_common::map::FastHashMap;
use librashader_common::{FilterMode, WrapMode};
use objc2::rc::Id;
use objc2::runtime::ProtocolObject;

use crate::error::{FilterChainError, Result};

pub struct SamplerSet {
    // todo: may need to deal with differences in mip filter.
    samplers:
        FastHashMap<(WrapMode, FilterMode, FilterMode), Id<ProtocolObject<dyn MTLSamplerState>>>,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(
        &self,
        wrap: WrapMode,
        filter: FilterMode,
        mipmap: FilterMode,
    ) -> &ProtocolObject<dyn MTLSamplerState> {
        // eprintln!("{wrap}, {filter}, {mip}");
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter x mipmap
        let id: &Id<ProtocolObject<dyn MTLSamplerState>> = unsafe {
            self.samplers
                .get(&(wrap, filter, mipmap))
                .unwrap_unchecked()
        };

        id.as_ref()
    }

    pub fn new(device: &ProtocolObject<dyn MTLDevice>) -> Result<SamplerSet> {
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
                    let descriptor = MTLSamplerDescriptor::new();
                    descriptor.setRAddressMode(MTLSamplerAddressMode::from(*wrap_mode));
                    descriptor.setSAddressMode(MTLSamplerAddressMode::from(*wrap_mode));
                    descriptor.setTAddressMode(MTLSamplerAddressMode::from(*wrap_mode));

                    descriptor.setMagFilter(MTLSamplerMinMagFilter::from(*filter_mode));

                    descriptor.setMinFilter(MTLSamplerMinMagFilter::from(*filter_mode));
                    descriptor.setMipFilter(filter_mode.mtl_mip(*mipmap_filter));
                    descriptor.setLodMinClamp(0.0);
                    descriptor.setLodMaxClamp(1000.0);
                    descriptor.setCompareFunction(MTLCompareFunctionNever);
                    descriptor.setMaxAnisotropy(1);
                    descriptor.setBorderColor(MTLSamplerBorderColorTransparentBlack);
                    descriptor.setNormalizedCoordinates(true);

                    let Some(sampler_state) = device.newSamplerStateWithDescriptor(&descriptor)
                    else {
                        return Err(FilterChainError::SamplerError(
                            *wrap_mode,
                            *filter_mode,
                            *mipmap_filter,
                        ));
                    };

                    samplers.insert((*wrap_mode, *filter_mode, *mipmap_filter), sampler_state);
                }
            }
        }

        // assert all samplers were created.
        assert_eq!(samplers.len(), wrap_modes.len() * 2 * 2);
        Ok(SamplerSet { samplers })
    }
}
