use crate::error;
use crate::error::assume_d3d11_init;
use std::slice;
use windows::core::PCSTR;
use windows::Win32::Graphics::Direct3D::Fxc::{
    D3DCompile, D3DCOMPILE_DEBUG, D3DCOMPILE_OPTIMIZATION_LEVEL3, D3DCOMPILE_SKIP_OPTIMIZATION,
};
use windows::Win32::Graphics::Direct3D::ID3DBlob;
use windows::Win32::Graphics::Direct3D11::*;
use windows::Win32::Graphics::Dxgi::Common::*;

/// wtf retroarch?
const DXGI_FORMAT_EX_A4R4G4B4_UNORM: DXGI_FORMAT = DXGI_FORMAT(1000);

const fn d3d11_format_fallback_list(format: DXGI_FORMAT) -> Option<&'static [DXGI_FORMAT]> {
    match format {
        DXGI_FORMAT_R32G32B32A32_FLOAT => Some(&[
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_R16G16B16A16_FLOAT => Some(&[
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_R8G8B8A8_UNORM => Some(&[
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_B8G8R8X8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB => Some(&[
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_B8G8R8X8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_B8G8R8A8_UNORM => Some(&[
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_B8G8R8X8_UNORM => Some(&[
            DXGI_FORMAT_B8G8R8X8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_B5G6R5_UNORM => Some(&[
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B8G8R8X8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_EX_A4R4G4B4_UNORM | DXGI_FORMAT_B4G4R4A4_UNORM => Some(&[
            DXGI_FORMAT_B4G4R4A4_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_A8_UNORM => Some(&[
            DXGI_FORMAT_A8_UNORM,
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        DXGI_FORMAT_R8_UNORM => Some(&[
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_A8_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_UNKNOWN,
        ]),
        _ => None,
    }
}

pub fn d3d11_get_closest_format(
    device: &ID3D11Device,
    format: DXGI_FORMAT,
    format_support_mask: i32,
) -> DXGI_FORMAT {
    let default_list = [format, DXGI_FORMAT_UNKNOWN];
    let format_support_list = d3d11_format_fallback_list(format).unwrap_or(&default_list);
    let format_support_mask = format_support_mask as u32;

    for supported in format_support_list {
        unsafe {
            if device
                .CheckFormatSupport(*supported)
                .is_ok_and(|supported_format| {
                    (supported_format & format_support_mask) == format_support_mask
                })
            {
                return *supported;
            }
        }
    }
    DXGI_FORMAT_UNKNOWN
}

pub fn d3d_compile_shader(source: &[u8], entry: &[u8], version: &[u8]) -> error::Result<ID3DBlob> {
    unsafe {
        let mut blob = None;
        D3DCompile(
            source.as_ptr().cast(),
            source.len(),
            None,
            None,
            None,
            PCSTR(entry.as_ptr()),
            PCSTR(version.as_ptr()),
            if cfg!(feature = "debug-shader") {
                D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
            } else {
                D3DCOMPILE_OPTIMIZATION_LEVEL3
            },
            0,
            &mut blob,
            None,
        )?;

        assume_d3d11_init!(blob, "D3DCompile");
        Ok(blob)
    }
}

pub type ShaderFactory<'a, L, T> = unsafe fn(
    &'a ID3D11Device,
    &[u8],
    linkage: L,
    Option<*mut Option<T>>,
) -> windows::core::Result<()>;

pub fn d3d11_compile_bound_shader<'a, T, L>(
    device: &'a ID3D11Device,
    blob: &ID3DBlob,
    linkage: L,
    factory: ShaderFactory<'a, L, T>,
) -> error::Result<T>
where
    L: windows::core::Param<ID3D11ClassLinkage>,
{
    unsafe {
        // SAFETY: slice as valid for as long as vs_blob is alive.
        let dxil =
            slice::from_raw_parts(blob.GetBufferPointer().cast::<u8>(), blob.GetBufferSize());

        let mut compiled = None;
        factory(device, dxil, linkage, Some(&mut compiled))?;
        assume_d3d11_init!(compiled, "CreateXXShader");
        Ok(compiled)
    }
}

pub fn d3d11_create_input_layout(
    device: &ID3D11Device,
    desc: &[D3D11_INPUT_ELEMENT_DESC],
    blob: &ID3DBlob,
) -> error::Result<ID3D11InputLayout> {
    unsafe {
        // SAFETY: slice as valid for as long as vs_blob is alive.
        let dxil =
            slice::from_raw_parts(blob.GetBufferPointer().cast::<u8>(), blob.GetBufferSize());
        let mut input_layout = None;
        device.CreateInputLayout(desc, dxil, Some(&mut input_layout))?;
        assume_d3d11_init!(input_layout, "CreateInputLayout");
        Ok(input_layout)
    }
}
