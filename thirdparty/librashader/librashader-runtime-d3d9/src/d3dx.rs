use std::ffi::c_void;
use windows::core::imp::BOOL;
use windows::core::{IntoParam, HRESULT, PCSTR};
use windows::Win32::Graphics::Direct3D::{ID3DBlob, ID3DInclude, D3D_SHADER_MACRO};

const D3DERR_INVALIDCALL: u32 = 0x8876086c;

#[allow(dead_code)]
pub mod raw {
    use super::*;

    #[link(name = "D3DX9_43", kind = "raw-dylib")]
    extern "system" {
        pub fn D3DXGetShaderVersion(p_function: *const c_void) -> u32;
        pub fn D3DXGetShaderSize(p_function: *const c_void) -> u32;
        pub fn D3DXGetShaderConstantTable(
            p_function: *const c_void,
            ppconstanttable: *mut *mut c_void,
        ) -> windows::core::HRESULT;

        pub(super) fn D3DXCompileShader(
            psrcdata: *const c_void,
            srcdatasize: usize,
            pdefines: *const D3D_SHADER_MACRO,
            pinclude: *mut c_void,
            pfunctionname: PCSTR,
            pprofile: PCSTR,
            flags: u32,
            ppcode: *mut *mut c_void,
            pperrormsgs: *mut *mut c_void,
            ppconstantable: *mut *mut c_void,
        ) -> windows::core::HRESULT;
    }
}

#[allow(dead_code)]
#[allow(non_snake_case)]
#[inline]
pub unsafe fn D3DXCompileShader<P1, P2, P3>(
    psrcdata: *const c_void,
    srcdatasize: usize,
    pdefines: Option<*const D3D_SHADER_MACRO>,
    pinclude: P1,
    pfunctioname: P2,
    pprofile: P3,
    flags: u32,
    ppcode: *mut Option<ID3DBlob>,
    pperrormsgs: Option<*mut Option<ID3DBlob>>,
    ppconstanttable: Option<*mut Option<ID3DXConstantTable>>,
) -> ::windows::core::Result<()>
where
    P1: IntoParam<ID3DInclude>,
    P2: IntoParam<PCSTR>,
    P3: IntoParam<PCSTR>,
{
    raw::D3DXCompileShader(
        psrcdata,
        srcdatasize,
        ::core::mem::transmute(pdefines.unwrap_or(::std::ptr::null())),
        pinclude.into_param().abi(),
        pfunctioname.into_param().abi(),
        pprofile.into_param().abi(),
        flags,
        ::core::mem::transmute(ppcode),
        ::core::mem::transmute(pperrormsgs.unwrap_or(::std::ptr::null_mut())),
        ::core::mem::transmute(ppconstanttable.unwrap_or(::std::ptr::null_mut())),
    )
    .ok()
}

#[repr(transparent)]
#[derive(PartialEq, Eq, Debug, Clone)]
pub struct ID3DXConstantTable(windows::core::IUnknown);

#[allow(dead_code)]
impl ID3DXConstantTable {
    #[allow(non_snake_case)]
    pub unsafe fn GetShaderConstantTable(
        p_function: *const c_void,
    ) -> windows::core::Result<ID3DXConstantTable> {
        let mut result__ = ::std::mem::zeroed();
        raw::D3DXGetShaderConstantTable(p_function, &mut result__).from_abi(result__)
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetBufferPointer(&self) -> *mut c_void {
        (windows::core::Interface::vtable(self).GetBufferPointer)(windows::core::Interface::as_raw(
            self,
        ))
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetBufferSize(&self) -> usize {
        (windows::core::Interface::vtable(self).GetBufferSize)(windows::core::Interface::as_raw(
            self,
        ))
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetDesc(&self) -> windows::core::Result<D3DXCONSTANTTABLE_DESC> {
        let mut result__ = ::std::mem::MaybeUninit::<D3DXCONSTANTTABLE_DESC>::zeroed();
        (windows::core::Interface::vtable(self).GetDesc)(
            windows::core::Interface::as_raw(self),
            result__.as_mut_ptr(),
        )
        .ok()?;

        Ok(result__.assume_init())
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetConstant(
        &self,
        hconstant: Option<D3DXHANDLE>,
        index: u32,
    ) -> windows::core::Result<D3DXHANDLE> {
        let handle = (windows::core::Interface::vtable(self).GetConstant)(
            windows::core::Interface::as_raw(self),
            hconstant.unwrap_or(D3DXHANDLE(std::ptr::null())),
            index,
        );

        if handle.0 as u32 == D3DERR_INVALIDCALL {
            return Err(HRESULT(D3DERR_INVALIDCALL as i32).into());
        }

        Ok(handle)
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetConstantDesc(
        &self,
        handle: D3DXHANDLE,
        pdesc: *mut D3DXCONSTANT_DESC,
        pcount: *mut u32,
    ) -> windows::core::Result<()> {
        (windows::core::Interface::vtable(self).GetConstantDesc)(
            windows::core::Interface::as_raw(self),
            handle,
            pdesc,
            pcount,
        )
        .ok()
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetConstantByName<P>(
        &self,
        hconstant: Option<D3DXHANDLE>,
        pname: P,
    ) -> windows::core::Result<D3DXHANDLE>
    where
        P: IntoParam<PCSTR>,
    {
        let handle = (windows::core::Interface::vtable(self).GetConstantByName)(
            windows::core::Interface::as_raw(self),
            hconstant.unwrap_or(D3DXHANDLE(std::ptr::null())),
            pname.into_param().abi(),
        );

        if handle.0 as u32 == D3DERR_INVALIDCALL {
            return Err(HRESULT(D3DERR_INVALIDCALL as i32).into());
        }

        Ok(handle)
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetConstantElement(
        &self,
        hconstant: D3DXHANDLE,
        index: u32,
    ) -> windows::core::Result<D3DXHANDLE> {
        let handle = (windows::core::Interface::vtable(self).GetConstantElement)(
            windows::core::Interface::as_raw(self),
            hconstant,
            index,
        );

        if handle.0 as u32 == D3DERR_INVALIDCALL {
            return Err(HRESULT(D3DERR_INVALIDCALL as i32).into());
        }

        Ok(handle)
    }

    #[allow(non_snake_case)]
    pub unsafe fn GetSamplerIndex(&self, hconstant: Option<D3DXHANDLE>) -> u32 {
        (windows::core::Interface::vtable(self).GetSamplerIndex)(
            windows::core::Interface::as_raw(self),
            hconstant.unwrap_or(D3DXHANDLE(std::ptr::null())),
        )
    }
}

impl windows::core::CanInto<windows::core::IUnknown> for ID3DXConstantTable {}
unsafe impl windows::core::Interface for ID3DXConstantTable {
    type Vtable = ID3DXConstantTable_Vtbl;
}

#[repr(C)]
#[doc(hidden)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
pub struct ID3DXConstantTable_Vtbl {
    pub base__: windows::core::IUnknown_Vtbl,
    #[allow(non_snake_case)]
    pub GetBufferPointer: unsafe extern "system" fn(this: *mut c_void) -> *mut c_void,
    #[allow(non_snake_case)]
    pub GetBufferSize: unsafe extern "system" fn(this: *mut c_void) -> usize,
    #[allow(non_snake_case)]
    pub GetDesc: unsafe extern "system" fn(
        this: *mut c_void,
        pdesc: *mut D3DXCONSTANTTABLE_DESC,
    ) -> windows::core::HRESULT,
    #[allow(non_snake_case)]
    pub GetConstantDesc: unsafe extern "system" fn(
        this: *mut c_void,
        hconstant: D3DXHANDLE,
        pdesc: *mut D3DXCONSTANT_DESC,
        pcount: *mut u32,
    ) -> windows::core::HRESULT,
    #[allow(non_snake_case)]
    pub GetSamplerIndex: unsafe extern "system" fn(this: *mut c_void, hconstant: D3DXHANDLE) -> u32,
    #[allow(non_snake_case)]
    pub GetConstant: unsafe extern "system" fn(
        this: *mut c_void,
        hconstant: D3DXHANDLE,
        index: u32,
    ) -> D3DXHANDLE,
    #[allow(non_snake_case)]
    pub GetConstantByName: unsafe extern "system" fn(
        this: *mut c_void,
        hconstant: D3DXHANDLE,
        pname: PCSTR,
    ) -> D3DXHANDLE,
    #[allow(non_snake_case)]
    pub GetConstantElement: unsafe extern "system" fn(
        this: *mut c_void,
        hconstant: D3DXHANDLE,
        index: u32,
    ) -> D3DXHANDLE,
    #[allow(non_snake_case)]
    pub SetDefaults: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
    ) -> windows::core::HRESULT,
    #[allow(non_snake_case)]
    pub SetValue: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        hconstant: D3DXHANDLE,
        pdata: *mut c_void,
        bytes: u32,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetBool:
        unsafe extern "system" fn(this: *mut c_void, pdevice: *mut c_void, b: BOOL) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetBoolArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        b: *const BOOL,
        count: u32,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetInt:
        unsafe extern "system" fn(this: *mut c_void, pdevice: *mut c_void, n: i32) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetIntArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        n: *const i32,
        count: u32,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetFloat: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        f: f32,
    ) -> windows::core::HRESULT,
    #[allow(non_snake_case)]
    pub SetFloatArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        b: *const f32,
        count: u32,
    ) -> windows::core::HRESULT,
    #[allow(non_snake_case)]
    pub SetVector: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        pvector: *const ::windows::Foundation::Numerics::Vector4,
    ) -> windows::core::HRESULT,
    #[allow(non_snake_case)]
    pub SetVectorArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        pvector: *const ::windows::Foundation::Numerics::Vector4,
        count: u32,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetMatrix: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        pmatrix: *const ::windows::Foundation::Numerics::Matrix4x4,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetMatrixArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        pmatrix: *const ::windows::Foundation::Numerics::Matrix4x4,
        count: u32,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetMatrixPointerArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        ppmatrix: *const *const ::windows::Foundation::Numerics::Matrix4x4,
        count: u32,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetMatrixTranspose: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        pmatrix: *const ::windows::Foundation::Numerics::Matrix4x4,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetMatrixTransposeArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        pmatrix: *const ::windows::Foundation::Numerics::Matrix4x4,
        count: u32,
    ) -> HRESULT,
    #[allow(non_snake_case)]
    pub SetMatrixTransposePointerArray: unsafe extern "system" fn(
        this: *mut c_void,
        pdevice: *mut c_void,
        ppmatrix: *const *const ::windows::Foundation::Numerics::Matrix4x4,
        count: u32,
    ) -> HRESULT,
}

#[repr(C)]
#[derive(Clone, Debug)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
pub struct D3DXCONSTANTTABLE_DESC {
    pub Creator: PCSTR,
    pub Version: u32,
    pub Constants: u32,
}

#[repr(C)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[derive(Debug)]
pub struct D3DXCONSTANT_DESC {
    pub Name: PCSTR,
    pub RegisterSet: D3DXREGISTER_SET,
    pub RegisterIndex: u32,
    pub RegisterCount: u32,
    pub Class: D3DXPARAMETER_CLASS,
    pub Type: D3DXPARAMETER_TYPE,
    pub Rows: u32,
    pub Columns: u32,
    pub Elements: u32,
    pub StructMembers: u32,
    pub Bytes: u32,
    pub DefaultValue: *const c_void,
}

#[repr(u32)]
#[derive(PartialEq, Eq, Debug)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
pub enum D3DXREGISTER_SET {
    D3DXRS_BOOL = 0,
    D3DXRS_INT4 = 1,
    D3DXRS_FLOAT4 = 2,
    D3DXRS_SAMPLER = 3,
    D3DXRS_FORCE_DWORD = 0x7fffffff,
}

#[repr(u32)]
#[derive(PartialEq, Eq, Debug)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
pub enum D3DXPARAMETER_CLASS {
    D3DXPC_SCALAR = 0,
    D3DXPC_VECTOR = 1,
    D3DXPC_MATRIX_ROWS = 2,
    D3DXPC_MATRIX_COLUMNS = 3,
    D3DXPC_OBJECT = 4,
    D3DXPC_STRUCT = 5,
    D3DXPC_FORCE_DWORD = 0x7fffffff,
}

#[repr(u32)]
#[derive(Debug)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
pub enum D3DXPARAMETER_TYPE {
    D3DXPT_VOID = 0,
    D3DXPT_BOOL = 1,
    D3DXPT_INT = 2,
    D3DXPT_FLOAT = 3,
    D3DXPT_STRING = 4,
    D3DXPT_TEXTURE = 5,
    D3DXPT_TEXTURE1D = 6,
    D3DXPT_TEXTURE2D = 7,
    D3DXPT_TEXTURE3D = 8,
    D3DXPT_TEXTURECUBE = 9,
    D3DXPT_SAMPLER = 10,
    D3DXPT_SAMPLER1D = 11,
    D3DXPT_SAMPLER2D = 12,
    D3DXPT_SAMPLER3D = 13,
    D3DXPT_SAMPLERCUBE = 14,
    D3DXPT_PIXELSHADER = 15,
    D3DXPT_VERTEXSHADER = 16,
    D3DXPT_PIXELFRAGMENT = 17,
    D3DXPT_VERTEXFRAGMENT = 18,
    D3DXPT_UNSUPPORTED = 19,
    D3DXPT_FORCE_DWORD = 0x7fffffff,
}

#[repr(transparent)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct D3DXHANDLE(pub *const c_void);

unsafe impl windows::core::ComInterface for ID3DXConstantTable {
    const IID: windows::core::GUID =
        windows::core::GUID::from_u128(0xab3c758f_93e_4356_b7_62_4d_b1_8f_1b_3a1);
}
