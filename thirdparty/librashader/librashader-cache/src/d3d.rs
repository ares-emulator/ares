//! Cache implementations for D3D blob types that need to live
//! here because of the orphan rule.

use crate::{CacheKey, Cacheable};
use windows::core::ComInterface;

impl CacheKey for windows::Win32::Graphics::Direct3D::ID3DBlob {
    fn hash_bytes(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts(self.GetBufferPointer().cast(), self.GetBufferSize()) }
    }
}

impl CacheKey for windows::Win32::Graphics::Direct3D::Dxc::IDxcBlob {
    fn hash_bytes(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts(self.GetBufferPointer().cast(), self.GetBufferSize()) }
    }
}

impl Cacheable for windows::Win32::Graphics::Direct3D::ID3DBlob {
    fn from_bytes(bytes: &[u8]) -> Option<Self> {
        let Some(blob) =
            (unsafe { windows::Win32::Graphics::Direct3D::Fxc::D3DCreateBlob(bytes.len()).ok() })
        else {
            return None;
        };

        let slice = unsafe {
            std::slice::from_raw_parts_mut(blob.GetBufferPointer().cast(), blob.GetBufferSize())
        };

        slice.copy_from_slice(bytes);

        Some(blob)
    }

    fn to_bytes(&self) -> Option<Vec<u8>> {
        let slice = unsafe {
            std::slice::from_raw_parts(self.GetBufferPointer().cast(), self.GetBufferSize())
        };

        Some(Vec::from(slice))
    }
}

impl Cacheable for windows::Win32::Graphics::Direct3D::Dxc::IDxcBlob {
    fn from_bytes(bytes: &[u8]) -> Option<Self> {
        let Some(blob) = (unsafe {
            windows::Win32::Graphics::Direct3D::Dxc::DxcCreateInstance(
                &windows::Win32::Graphics::Direct3D::Dxc::CLSID_DxcLibrary,
            )
            .and_then(
                |library: windows::Win32::Graphics::Direct3D::Dxc::IDxcUtils| {
                    library.CreateBlob(
                        bytes.as_ptr().cast(),
                        bytes.len() as u32,
                        windows::Win32::Graphics::Direct3D::Dxc::DXC_CP(0),
                    )
                },
            )
            .ok()
        }) else {
            return None;
        };

        Some(blob.cast().ok()?)
    }

    fn to_bytes(&self) -> Option<Vec<u8>> {
        let slice = unsafe {
            std::slice::from_raw_parts(self.GetBufferPointer().cast(), self.GetBufferSize())
        };

        Some(Vec::from(slice))
    }
}
