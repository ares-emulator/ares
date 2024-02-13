use gl::types::{GLenum, GLint, GLuint};
use librashader_common::{FilterMode, WrapMode};
use rustc_hash::FxHashMap;

pub struct SamplerSet {
    // todo: may need to deal with differences in mip filter.
    samplers: FxHashMap<(WrapMode, FilterMode, FilterMode), GLuint>,
}

impl SamplerSet {
    #[inline(always)]
    pub fn get(&self, wrap: WrapMode, filter: FilterMode, mipmap: FilterMode) -> GLuint {
        // SAFETY: the sampler set is complete for the matrix
        // wrap x filter x mipmap
        unsafe {
            *self
                .samplers
                .get(&(wrap, filter, mipmap))
                .unwrap_unchecked()
        }
    }

    fn make_sampler(sampler: GLuint, wrap: WrapMode, filter: FilterMode, mip: FilterMode) {
        unsafe {
            gl::SamplerParameteri(sampler, gl::TEXTURE_WRAP_S, GLenum::from(wrap) as GLint);
            gl::SamplerParameteri(sampler, gl::TEXTURE_WRAP_T, GLenum::from(wrap) as GLint);
            gl::SamplerParameteri(
                sampler,
                gl::TEXTURE_MAG_FILTER,
                GLenum::from(filter) as GLint,
            );

            gl::SamplerParameteri(sampler, gl::TEXTURE_MIN_FILTER, filter.gl_mip(mip) as GLint);
        }
    }

    pub fn new() -> SamplerSet {
        let mut samplers = FxHashMap::default();
        let wrap_modes = &[
            WrapMode::ClampToBorder,
            WrapMode::ClampToEdge,
            WrapMode::Repeat,
            WrapMode::MirroredRepeat,
        ];
        for wrap_mode in wrap_modes {
            for filter_mode in &[FilterMode::Linear, FilterMode::Nearest] {
                for mip_filter in &[FilterMode::Linear, FilterMode::Nearest] {
                    let mut sampler = 0;
                    unsafe {
                        gl::GenSamplers(1, &mut sampler);
                        SamplerSet::make_sampler(sampler, *wrap_mode, *filter_mode, *mip_filter);

                        samplers.insert((*wrap_mode, *filter_mode, *mip_filter), sampler);
                    }
                }
            }
        }

        // assert all samplers were created.
        assert_eq!(samplers.len(), wrap_modes.len() * 2 * 2);
        SamplerSet { samplers }
    }
}
