use crate::error::Result;
use librashader_common::map::FastHashMap;
use librashader_common::{FilterMode, WrapMode};

use windows::Win32::Graphics::Direct3D9::{
    IDirect3DDevice9, D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_ADDRESSW, D3DSAMP_MAGFILTER,
    D3DSAMP_MINFILTER, D3DSAMP_MIPFILTER, D3DTEXTUREADDRESS, D3DTEXTUREFILTER,
};

pub struct SamplerSet {
    samplers: FastHashMap<
        (WrapMode, FilterMode, FilterMode),
        Box<dyn Fn(&IDirect3DDevice9, u32) -> Result<()>>,
    >,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(
        &self,
        wrap: WrapMode,
        filter: FilterMode,
        mip_filter: FilterMode,
    ) -> &dyn Fn(&IDirect3DDevice9, u32) -> Result<()> {
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter x mipfilter
        unsafe {
            &*self
                .samplers
                .get(&(wrap, filter, mip_filter))
                .unwrap_unchecked()
        }
    }

    pub fn new() -> Result<SamplerSet> {
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
                    let sampler: Box<dyn Fn(&IDirect3DDevice9, u32) -> Result<()>> =
                        Box::new(|device: &IDirect3DDevice9, index| {
                            unsafe {
                                let wrap_mode = *wrap_mode;
                                let filter_mode = *filter_mode;
                                let mip_filter = *mip_filter;

                                device.SetSamplerState(
                                    index,
                                    D3DSAMP_ADDRESSU,
                                    D3DTEXTUREADDRESS::from(wrap_mode).0 as u32,
                                )?;
                                device.SetSamplerState(
                                    index,
                                    D3DSAMP_ADDRESSV,
                                    D3DTEXTUREADDRESS::from(wrap_mode).0 as u32,
                                )?;
                                device.SetSamplerState(
                                    index,
                                    D3DSAMP_ADDRESSW,
                                    D3DTEXTUREADDRESS::from(wrap_mode).0 as u32,
                                )?;

                                device.SetSamplerState(
                                    index,
                                    D3DSAMP_MAGFILTER,
                                    D3DTEXTUREFILTER::from(filter_mode).0 as u32,
                                )?;
                                device.SetSamplerState(
                                    index,
                                    D3DSAMP_MINFILTER,
                                    D3DTEXTUREFILTER::from(mip_filter).0 as u32,
                                )?;
                                device.SetSamplerState(
                                    index,
                                    D3DSAMP_MIPFILTER,
                                    D3DTEXTUREFILTER::from(mip_filter).0 as u32,
                                )?;
                            }

                            Ok(())
                        });

                    samplers.insert((*wrap_mode, *filter_mode, *mip_filter), sampler);
                }
            }
        }

        assert_eq!(samplers.len(), wrap_modes.len() * 2 * 2);
        Ok(SamplerSet { samplers })
    }
}
