use crate::error;
use std::mem::ManuallyDrop;
use widestring::{u16cstr, U16CStr};
use windows::core::{Interface, PCWSTR};
use windows::Win32::Graphics::Direct3D::Dxc::{
    DxcValidatorFlags_InPlaceEdit, IDxcBlob, IDxcCompiler, IDxcUtils, IDxcValidator, DXC_CP,
    DXC_CP_UTF8,
};

use crate::resource::ResourceHandleStrategy;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, D3D12_FEATURE_DATA_FORMAT_SUPPORT,
    D3D12_FEATURE_FORMAT_SUPPORT, D3D12_RESOURCE_BARRIER, D3D12_RESOURCE_BARRIER_0,
    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE,
    D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_STATES,
    D3D12_RESOURCE_TRANSITION_BARRIER,
};
use windows::Win32::Graphics::Dxgi::Common::*;

/// wtf retroarch?
const DXGI_FORMAT_EX_A4R4G4B4_UNORM: DXGI_FORMAT = DXGI_FORMAT(1000);

const fn d3d12_format_fallback_list(format: DXGI_FORMAT) -> Option<&'static [DXGI_FORMAT]> {
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

pub fn d3d12_get_closest_format(
    device: &ID3D12Device,
    format_support: D3D12_FEATURE_DATA_FORMAT_SUPPORT,
) -> DXGI_FORMAT {
    let format = format_support.Format;
    let default_list = [format, DXGI_FORMAT_UNKNOWN];
    let format_support_list = d3d12_format_fallback_list(format).unwrap_or(&default_list);

    for supported in format_support_list {
        unsafe {
            let mut support = D3D12_FEATURE_DATA_FORMAT_SUPPORT {
                Format: *supported,
                ..Default::default()
            };
            if device
                .CheckFeatureSupport(
                    D3D12_FEATURE_FORMAT_SUPPORT,
                    &mut support as *mut _ as *mut _,
                    std::mem::size_of::<D3D12_FEATURE_DATA_FORMAT_SUPPORT>() as u32,
                )
                .is_ok()
                && (support.Support1 & format_support.Support1) == format_support.Support1
                && (support.Support2 & format_support.Support2) == format_support.Support2
            {
                return *supported;
            }
        }
    }
    DXGI_FORMAT_UNKNOWN
}

pub fn dxc_compile_shader(
    library: &IDxcUtils,
    compiler: &IDxcCompiler,
    source: impl AsRef<[u8]>,
    profile: &U16CStr,
) -> error::Result<IDxcBlob> {
    let include = unsafe { library.CreateDefaultIncludeHandler()? };
    let source = source.as_ref();
    let blob = unsafe {
        library.CreateBlobFromPinned(source.as_ptr().cast(), source.len() as u32, DXC_CP_UTF8)?
    };

    unsafe {
        let result = compiler.Compile(
            &blob,
            PCWSTR::null(),
            PCWSTR(u16cstr!("main").as_ptr()),
            PCWSTR(profile.as_ptr()),
            Some(&[PCWSTR(u16cstr!("-HV 2016").as_ptr())]),
            &[],
            &include,
        )?;

        let result = result.GetResult()?;

        {
            let blob = std::slice::from_raw_parts_mut(
                result.GetBufferPointer() as *mut u8,
                result.GetBufferSize(),
            );
            mach_siegbert_vogt_dxcsa::sign_in_place(blob);
        }

        Ok(result)
    }
}

pub fn dxc_validate_shader(
    library: &IDxcUtils,
    validator: &IDxcValidator,
    source: &[u8],
) -> error::Result<IDxcBlob> {
    let blob =
        unsafe { library.CreateBlob(source.as_ptr().cast(), source.len() as u32, DXC_CP(0))? };

    unsafe {
        let _result = validator.Validate(&blob, DxcValidatorFlags_InPlaceEdit)?;
        Ok(blob.cast()?)
    }
}

pub fn d3d12_get_resource_transition_subresource<S: ResourceHandleStrategy<T>, T>(
    resource: &T,
    before: D3D12_RESOURCE_STATES,
    after: D3D12_RESOURCE_STATES,
    subresource: u32,
) -> D3D12_RESOURCE_BARRIER {
    D3D12_RESOURCE_BARRIER {
        Type: D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        Flags: D3D12_RESOURCE_BARRIER_FLAG_NONE,
        Anonymous: D3D12_RESOURCE_BARRIER_0 {
            Transition: ManuallyDrop::new(D3D12_RESOURCE_TRANSITION_BARRIER {
                pResource: unsafe { S::obtain(resource) },
                Subresource: subresource,
                StateBefore: before,
                StateAfter: after,
            }),
        },
    }
}

#[inline(always)]
pub fn d3d12_resource_transition<S: ResourceHandleStrategy<T>, T>(
    cmd: &ID3D12GraphicsCommandList,
    resource: &T,
    before: D3D12_RESOURCE_STATES,
    after: D3D12_RESOURCE_STATES,
) -> [D3D12_RESOURCE_BARRIER; 1] {
    d3d12_resource_transition_subresource::<S, T>(
        cmd,
        resource,
        before,
        after,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
    )
}

#[must_use = "Resource Barrier must be disposed"]
#[inline(always)]
fn d3d12_resource_transition_subresource<S: ResourceHandleStrategy<T>, T>(
    cmd: &ID3D12GraphicsCommandList,
    resource: &T,
    before: D3D12_RESOURCE_STATES,
    after: D3D12_RESOURCE_STATES,
    subresource: u32,
) -> [D3D12_RESOURCE_BARRIER; 1] {
    let barrier = [d3d12_get_resource_transition_subresource::<S, T>(
        resource,
        before,
        after,
        subresource,
    )];
    unsafe { cmd.ResourceBarrier(&barrier) }
    barrier
}
