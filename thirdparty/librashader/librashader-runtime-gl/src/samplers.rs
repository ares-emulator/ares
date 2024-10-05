use crate::error;
use crate::error::FilterChainError;
use glow::HasContext;
use librashader_common::map::FastHashMap;
use librashader_common::{FilterMode, WrapMode};

pub struct SamplerSet {
    // todo: may need to deal with differences in mip filter.
    samplers: FastHashMap<(WrapMode, FilterMode, FilterMode), glow::Sampler>,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(&self, wrap: WrapMode, filter: FilterMode, mipmap: FilterMode) -> glow::Sampler {
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter x mipmap
        unsafe {
            *self
                .samplers
                .get(&(wrap, filter, mipmap))
                .unwrap_unchecked()
        }
    }

    fn make_sampler(
        context: &glow::Context,
        sampler: glow::Sampler,
        wrap: WrapMode,
        filter: FilterMode,
        mip: FilterMode,
    ) {
        unsafe {
            context.sampler_parameter_i32(sampler, glow::TEXTURE_WRAP_S, wrap.into());
            context.sampler_parameter_i32(sampler, glow::TEXTURE_WRAP_T, wrap.into());
            context.sampler_parameter_i32(sampler, glow::TEXTURE_MAG_FILTER, filter.into());
            context.sampler_parameter_i32(
                sampler,
                glow::TEXTURE_MIN_FILTER,
                filter.gl_mip(mip) as i32,
            );
        }
    }

    pub fn new(context: &glow::Context) -> error::Result<SamplerSet> {
        let mut samplers = FastHashMap::default();
        let wrap_modes = &[
            WrapMode::ClampToBorder,
            WrapMode::ClampToEdge,
            WrapMode::Repeat,
            WrapMode::MirroredRepeat,
        ];
        for wrap_mode in wrap_modes {
            for filter_mode in &[FilterMode::Linear, FilterMode::Nearest] {
                for mip_filter in &[FilterMode::Linear, FilterMode::Nearest] {
                    unsafe {
                        let sampler = context
                            .create_sampler()
                            .map_err(|_| FilterChainError::GlSamplerError)?;

                        SamplerSet::make_sampler(
                            context,
                            sampler,
                            *wrap_mode,
                            *filter_mode,
                            *mip_filter,
                        );

                        samplers.insert((*wrap_mode, *filter_mode, *mip_filter), sampler);
                    }
                }
            }
        }

        // assert all samplers were created.
        assert_eq!(samplers.len(), wrap_modes.len() * 2 * 2);
        Ok(SamplerSet { samplers })
    }
}
