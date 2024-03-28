use crate::error;
use windows::Win32::Graphics::Direct3D9::{IDirect3DDevice9, IDirect3DStateBlock9, D3DSBT_ALL};

pub struct D3D9State {
    state: IDirect3DStateBlock9,
}

impl D3D9State {
    pub fn new(device: &IDirect3DDevice9) -> error::Result<D3D9State> {
        let block = unsafe {
            let block = device.CreateStateBlock(D3DSBT_ALL)?;
            block.Capture()?;

            block
        };

        Ok(D3D9State { state: block })
    }
}

impl Drop for D3D9State {
    fn drop(&mut self) {
        if let Err(e) = unsafe { self.state.Apply() } {
            println!("librashader-runtime-d3d9: [warn] failed to restore state {e:?}")
        }
    }
}
