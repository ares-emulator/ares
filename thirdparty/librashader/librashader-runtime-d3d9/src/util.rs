use crate::error;
use crate::error::assume_d3d_init;

use std::mem::MaybeUninit;

use crate::binding::{ConstantDescriptor, RegisterAssignment, RegisterSet};
use crate::d3dx::{ID3DXConstantTable, D3DXCONSTANT_DESC, D3DXREGISTER_SET};
use librashader_common::map::FastHashMap;
use windows::core::PCSTR;
use windows::Win32::Graphics::Direct3D::Fxc::{D3DCompile, D3DCOMPILE_AVOID_FLOW_CONTROL};
use windows::Win32::Graphics::Direct3D::ID3DBlob;

// const fn d3d9_format_fallback_list(format: D3DFORMAT) -> Option<&'static [D3DFORMAT]> {
//     match format {
//         DXGI_FORMAT_R32G32B32A32_FLOAT => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R32G32B32A32_FLOAT,
//             DXGI_FORMAT_R16G16B16A16_FLOAT,
//             DXGI_FORMAT_R32G32B32_FLOAT,
//             DXGI_FORMAT_R11G11B10_FLOAT,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
//         DXGI_FORMAT_R16G16B16A16_FLOAT => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R16G16B16A16_FLOAT,
//             DXGI_FORMAT_R32G32B32A32_FLOAT,
//             DXGI_FORMAT_R32G32B32_FLOAT,
//             DXGI_FORMAT_R11G11B10_FLOAT,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
//         DXGI_FORMAT_R8G8B8A8_UNORM => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R8G8B8A8_UNORM,
//             DXGI_FORMAT_B8G8R8A8_UNORM,
//             DXGI_FORMAT_B8G8R8X8_UNORM,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
//         DXGI_FORMAT_R8G8B8A8_UNORM_SRGB => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
//             DXGI_FORMAT_R8G8B8A8_UNORM,
//             DXGI_FORMAT_B8G8R8A8_UNORM,
//             DXGI_FORMAT_B8G8R8X8_UNORM,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
//         DXGI_FORMAT_B8G8R8A8_UNORM => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_B8G8R8A8_UNORM,
//             DXGI_FORMAT_R8G8B8A8_UNORM,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
//         DXGI_FORMAT_B8G8R8X8_UNORM => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_B8G8R8X8_UNORM,
//             DXGI_FORMAT_B8G8R8A8_UNORM,
//             DXGI_FORMAT_R8G8B8A8_UNORM,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
//         DXGI_FORMAT_B5G6R5_UNORM => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_B5G6R5_UNORM,
//             DXGI_FORMAT_B8G8R8X8_UNORM,
//             DXGI_FORMAT_B8G8R8A8_UNORM,
//             DXGI_FORMAT_R8G8B8A8_UNORM,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
//         DXGI_FORMAT_EX_A4R4G4B4_UNORM | DXGI_FORMAT_B4G4R4A4_UNORM => Some(&[
//             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_B4G4R4A4_UNORM,
//             DXGI_FORMAT_B8G8R8A8_UNORM,
//             DXGI_FORMAT_R8G8B8A8_UNORM,
//             DXGI_FORMAT_UNKNOWN,
//         ]),
// //         DXGI_FORMAT_A8_UNORM => Some(&[
// //             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_A8_UNORM,
// //             DXGI_FORMAT_R8_UNORM,
// //             DXGI_FORMAT_R8G8_UNORM,
// //             DXGI_FORMAT_R8G8B8A8_UNORM,
// //             DXGI_FORMAT_B8G8R8A8_UNORM,
// //             DXGI_FORMAT_UNKNOWN,
// //         ]),
// //         DXGI_FORMAT_R8_UNORM => Some(&[
// //             windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_R8_UNORM,
// //             DXGI_FORMAT_A8_UNORM,
// //             DXGI_FORMAT_R8G8_UNORM,
// //             DXGI_FORMAT_R8G8B8A8_UNORM,
// //             DXGI_FORMAT_B8G8R8A8_UNORM,
// //             DXGI_FORMAT_UNKNOWN,
// //         ]),
// //         _ => None,
// //     }
// // }
//
// pub fn d3d9_get_closest_format(
//     device: &IDirect3DDevice9,
//     format: D3DFORMAT,
//     restype: D3DRESOURCETYPE,
//     format_support_mask: i32,
// ) -> error::Result<D3DFORMAT> {
//     let d3d9 = unsafe { device.GetDirect3D()? };
//     let (devtype, ordinal) = unsafe {
//         let mut params = Default::default();
//         device.GetCreationParameters(&mut params)?;
//         (params.DeviceType, params.AdapterOrdinal)
//     };
//
//     let default_list = [format, D3DFMT_UNKNOWN];
//     let format_support_list = d3d9_format_fallback_list(format).unwrap_or(&default_list);
//     let format_support_mask = format_support_mask as u32;
//
//     for supported in format_support_list {
//         unsafe {
//             if let Ok(_) = d3d9.CheckDeviceFormat(
//                 ordinal,devtype,
//                 format, format_support_mask,
//                 restype,
//                 format)
//             {
//                 return Ok(*supported);
//             }
//         }
//     }
//
//     Ok(D3DFMT_UNKNOWN)
// }

pub fn d3d_compile_shader(source: &[u8], entry: &[u8], version: &[u8]) -> error::Result<ID3DBlob> {
    unsafe {
        let mut blob = None;
        let mut errs = None;
        let res = D3DCompile(
            source.as_ptr().cast(),
            source.len(),
            None,
            None,
            None,
            PCSTR(entry.as_ptr()),
            PCSTR(version.as_ptr()),
            D3DCOMPILE_AVOID_FLOW_CONTROL,
            0,
            &mut blob,
            Some(&mut errs),
        );

        // let res = D3DXCompileShader(
        //     source.as_ptr().cast(),
        //     source.len(),
        //     None,
        //     None,
        //     PCSTR(entry.as_ptr()),
        //     PCSTR(version.as_ptr()),
        //     0,
        //     &mut blob,
        //     Some(&mut errs),
        //     None,
        // );

        // if let Some(errs) = errs {
        //     let str = std::slice::from_raw_parts(
        //         errs.GetBufferPointer().cast::<u8>(),
        //         errs.GetBufferSize(),
        //     );
        //     let str = std::ffi::CStr::from_bytes_until_nul(str).unwrap();
        //     // eprintln!("{}", str.to_str().unwrap());
        // }

        res?;
        assume_d3d_init!(blob, "D3DCompile");

        Ok(blob)
    }
}
pub fn d3d_reflect_shader(
    shader: ID3DBlob,
) -> error::Result<FastHashMap<String, ConstantDescriptor>> {
    unsafe {
        let table = ID3DXConstantTable::GetShaderConstantTable(shader.GetBufferPointer())?;
        let desc = table.GetDesc()?;

        let mut scratch = Vec::with_capacity(16);
        scratch.resize_with(16, MaybeUninit::zeroed);
        let mut assignments = FastHashMap::default();
        for assignment in 0..desc.Constants {
            let constant = table.GetConstant(None, assignment)?;
            let mut written = scratch.len() as u32;
            table.GetConstantDesc(constant, scratch.as_mut_ptr().cast(), &mut written)?;

            for i in 0..written as usize {
                let desc: MaybeUninit<D3DXCONSTANT_DESC> =
                    std::mem::replace(&mut scratch[i], MaybeUninit::zeroed());

                let desc = desc.assume_init();

                // Only cN and sN allowed.
                if desc.RegisterSet != D3DXREGISTER_SET::D3DXRS_FLOAT4
                    && desc.RegisterSet != D3DXREGISTER_SET::D3DXRS_SAMPLER
                {
                    continue;
                }
                let name = desc.Name.to_string()?;
                assignments.insert(
                    name,
                    ConstantDescriptor {
                        assignment: RegisterAssignment {
                            index: desc.RegisterIndex,
                            count: desc.RegisterCount,
                        },
                        set: if desc.RegisterSet == D3DXREGISTER_SET::D3DXRS_SAMPLER {
                            RegisterSet::Sampler
                        } else {
                            RegisterSet::Float
                        },
                    },
                );
            }
        }
        Ok(assignments)
    }
}
