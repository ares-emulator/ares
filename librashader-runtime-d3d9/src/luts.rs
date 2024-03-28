use crate::error;
use crate::error::assume_d3d_init;
use crate::texture::D3D9InputTexture;

use librashader_presets::TextureConfig;
use librashader_runtime::image::{Image, ARGB8};

use windows::Win32::Graphics::Direct3D9::{
    IDirect3DDevice9, D3DFMT_A8R8G8B8, D3DLOCKED_RECT, D3DPOOL_MANAGED,
};

#[derive(Debug, Clone)]
pub(crate) struct LutTexture(D3D9InputTexture);

impl AsRef<D3D9InputTexture> for LutTexture {
    fn as_ref(&self) -> &D3D9InputTexture {
        &self.0
    }
}

impl LutTexture {
    pub fn new(
        device: &IDirect3DDevice9,
        source: &Image<ARGB8>,
        config: &TextureConfig,
    ) -> error::Result<LutTexture> {
        let mut texture = None;
        unsafe {
            device.CreateTexture(
                source.size.width,
                source.size.height,
                if config.mipmap { 0 } else { 1 },
                0,
                D3DFMT_A8R8G8B8,
                D3DPOOL_MANAGED,
                &mut texture,
                std::ptr::null_mut(),
            )?;
        }
        assume_d3d_init!(texture, "CreateTexture");

        unsafe {
            let mut lock = D3DLOCKED_RECT::default();
            texture.LockRect(0, &mut lock, std::ptr::null_mut(), 0)?;
            std::ptr::copy_nonoverlapping(
                source.bytes.as_ptr(),
                lock.pBits.cast(),
                source.bytes.len(),
            );
            texture.UnlockRect(0)?;

            if config.mipmap {
                texture.GenerateMipSubLevels();
            }
        }

        Ok(LutTexture(D3D9InputTexture {
            handle: texture,
            filter: config.filter_mode,
            wrap: config.wrap_mode,
            mipmode: config.filter_mode,
            is_srgb: false,
        }))
    }
}
