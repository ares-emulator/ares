//! librashader error C API. (`libra_error_*`).
use std::any::Any;
use std::ffi::{c_char, CString};
use std::mem::MaybeUninit;
use std::ptr::NonNull;
use thiserror::Error;

/// The error type for librashader C API.
#[non_exhaustive]
#[derive(Error, Debug)]
pub enum LibrashaderError {
    /// An unknown error or panic occurred.
    #[error("There was an unknown error.")]
    UnknownError(Box<dyn Any + Send + 'static>),

    /// An invalid parameter (likely null), was passed.
    #[error("The parameter was null or invalid.")]
    InvalidParameter(&'static str),

    /// The string provided was not valid UTF-8.
    #[error("The provided string was not valid UTF8.")]
    InvalidString(#[from] std::str::Utf8Error),

    /// An error occurred in the preset parser.
    #[error("There was an error parsing the preset.")]
    PresetError(#[from] librashader::presets::ParsePresetError),

    /// An error occurred in the shader preprocessor.
    #[error("There was an error preprocessing the shader source.")]
    PreprocessError(#[from] librashader::preprocess::PreprocessError),

    /// An error occurred in the shader compiler.
    #[error("There was an error compiling the shader source.")]
    ShaderCompileError(#[from] librashader::reflect::ShaderCompileError),

    /// An error occrred when validating and reflecting the shader.
    #[error("There was an error reflecting the shader source.")]
    ShaderReflectError(#[from] librashader::reflect::ShaderReflectError),

    /// An invalid shader parameter name was provided.
    #[error("The provided parameter name was invalid.")]
    UnknownShaderParameter(*const c_char),

    /// An error occurred with the OpenGL filter chain.
    #[cfg(feature = "runtime-opengl")]
    #[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime-opengl")))]
    #[error("There was an error in the OpenGL filter chain.")]
    OpenGlFilterError(#[from] librashader::runtime::gl::error::FilterChainError),

    /// An error occurred with the Direct3D 11 filter chain.
    #[cfg(all(target_os = "windows", feature = "runtime-d3d11"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_os = "windows", feature = "runtime-d3d11")))
    )]
    #[error("There was an error in the D3D11 filter chain.")]
    D3D11FilterError(#[from] librashader::runtime::d3d11::error::FilterChainError),

    /// An error occurred with the Direct3D 12 filter chain.
    #[cfg(all(target_os = "windows", feature = "runtime-d3d12"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_os = "windows", feature = "runtime-d3d12")))
    )]
    #[error("There was an error in the D3D12 filter chain.")]
    D3D12FilterError(#[from] librashader::runtime::d3d12::error::FilterChainError),

    /// An error occurred with the Direct3D 9 filter chain.
    #[cfg(all(target_os = "windows", feature = "runtime-d3d9"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_os = "windows", feature = "runtime-d3d9")))
    )]
    #[error("There was an error in the D3D9 filter chain.")]
    D3D9FilterError(#[from] librashader::runtime::d3d9::error::FilterChainError),

    /// An error occurred with the Vulkan filter chain.

    #[cfg(feature = "runtime-vulkan")]
    #[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime-vulkan")))]
    #[error("There was an error in the Vulkan filter chain.")]
    VulkanFilterError(#[from] librashader::runtime::vk::error::FilterChainError),

    /// An error occurred with the Metal filter chain.
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_vendor = "apple", feature = "runtime-metal")))
    )]
    #[cfg(all(target_vendor = "apple", feature = "runtime-metal"))]
    #[error("There was an error in the Metal filter chain.")]
    MetalFilterError(#[from] librashader::runtime::mtl::error::FilterChainError),
    /// This error is unreachable.
    #[error("This error is not reachable")]
    Infallible(#[from] std::convert::Infallible),
}

/// Error codes for librashader error types.
#[repr(i32)]
pub enum LIBRA_ERRNO {
    /// Error code for an unknown error.
    UNKNOWN_ERROR = 0,

    /// Error code for an invalid parameter.
    INVALID_PARAMETER = 1,

    /// Error code for an invalid (non-UTF8) string.
    INVALID_STRING = 2,

    /// Error code for a preset parser error.
    PRESET_ERROR = 3,

    /// Error code for a preprocessor error.
    PREPROCESS_ERROR = 4,

    /// Error code for a shader parameter error.
    SHADER_PARAMETER_ERROR = 5,

    /// Error code for a reflection error.
    REFLECT_ERROR = 6,

    /// Error code for a runtime error.
    RUNTIME_ERROR = 7,
}

// Nothing here can use extern_fn because they are lower level than libra_error_t.

/// Function pointer definition for libra_error_errno
pub type PFN_libra_error_errno = extern "C" fn(error: libra_error_t) -> LIBRA_ERRNO;
#[no_mangle]
/// Get the error code corresponding to this error object.
///
/// ## Safety
///   - `error` must be valid and initialized.
pub unsafe extern "C" fn libra_error_errno(error: libra_error_t) -> LIBRA_ERRNO {
    let Some(error) = error else {
        return LIBRA_ERRNO::UNKNOWN_ERROR;
    };

    unsafe { error.as_ref().get_code() }
}

/// Function pointer definition for libra_error_print
pub type PFN_libra_error_print = extern "C" fn(error: libra_error_t) -> i32;
#[no_mangle]
/// Print the error message.
///
/// If `error` is null, this function does nothing and returns 1. Otherwise, this function returns 0.
/// ## Safety
///   - `error` must be a valid and initialized instance of `libra_error_t`.
pub unsafe extern "C" fn libra_error_print(error: libra_error_t) -> i32 {
    let Some(error) = error else { return 1 };
    unsafe {
        let error = error.as_ref();
        println!("{error:?}: {error}");
    }
    0
}

/// Function pointer definition for libra_error_free
pub type PFN_libra_error_free = extern "C" fn(error: *mut libra_error_t) -> i32;
#[no_mangle]
/// Frees any internal state kept by the error.
///
/// If `error` is null, this function does nothing and returns 1. Otherwise, this function returns 0.
/// The resulting error object becomes null.
/// ## Safety
///   - `error` must be null or a pointer to a valid and initialized instance of `libra_error_t`.
pub unsafe extern "C" fn libra_error_free(error: *mut libra_error_t) -> i32 {
    if error.is_null() {
        return 1;
    }

    let error = unsafe { &mut *error };
    let error = error.take();
    let Some(error) = error else {
        return 1;
    };

    unsafe { drop(Box::from_raw(error.as_ptr())) }
    0
}

/// Function pointer definition for libra_error_write
pub type PFN_libra_error_write =
    extern "C" fn(error: libra_error_t, out: *mut MaybeUninit<*mut c_char>) -> i32;
#[no_mangle]
/// Writes the error message into `out`
///
/// If `error` is null, this function does nothing and returns 1. Otherwise, this function returns 0.
/// ## Safety
///   - `error` must be a valid and initialized instance of `libra_error_t`.
///   - `out` must be a non-null pointer. The resulting string must not be modified.
pub unsafe extern "C" fn libra_error_write(
    error: libra_error_t,
    out: *mut MaybeUninit<*mut c_char>,
) -> i32 {
    let Some(error) = error else { return 1 };
    if out.is_null() {
        return 1;
    }

    unsafe {
        let error = error.as_ref();
        let Ok(cstring) = CString::new(format!("{error:?}: {error}")) else {
            return 1;
        };

        out.write(MaybeUninit::new(cstring.into_raw()))
    }
    0
}

/// Function pointer definition for libra_error_free_string
pub type PFN_libra_error_free_string = extern "C" fn(out: *mut *mut c_char) -> i32;
#[no_mangle]
/// Frees an error string previously allocated by `libra_error_write`.
///
/// After freeing, the pointer will be set to null.
/// ## Safety
///   - If `libra_error_write` is not null, it must point to a string previously returned by `libra_error_write`.
///     Attempting to free anything else, including strings or objects from other librashader functions, is immediate
///     Undefined Behaviour.
pub unsafe extern "C" fn libra_error_free_string(out: *mut *mut c_char) -> i32 {
    if out.is_null() {
        return 1;
    }

    unsafe {
        let ptr = out.read();
        *out = std::ptr::null_mut();
        drop(CString::from_raw(ptr))
    }
    0
}

impl LibrashaderError {
    pub(crate) const fn get_code(&self) -> LIBRA_ERRNO {
        match self {
            LibrashaderError::UnknownError(_) => LIBRA_ERRNO::UNKNOWN_ERROR,
            LibrashaderError::InvalidParameter(_) => LIBRA_ERRNO::INVALID_PARAMETER,
            LibrashaderError::InvalidString(_) => LIBRA_ERRNO::INVALID_STRING,
            LibrashaderError::PresetError(_) => LIBRA_ERRNO::PRESET_ERROR,
            LibrashaderError::PreprocessError(_) => LIBRA_ERRNO::PREPROCESS_ERROR,
            LibrashaderError::ShaderCompileError(_) | LibrashaderError::ShaderReflectError(_) => {
                LIBRA_ERRNO::REFLECT_ERROR
            }
            LibrashaderError::UnknownShaderParameter(_) => LIBRA_ERRNO::SHADER_PARAMETER_ERROR,
            #[cfg(feature = "runtime-opengl")]
            LibrashaderError::OpenGlFilterError(_) => LIBRA_ERRNO::RUNTIME_ERROR,
            #[cfg(all(target_os = "windows", feature = "runtime-d3d11"))]
            LibrashaderError::D3D11FilterError(_) => LIBRA_ERRNO::RUNTIME_ERROR,
            #[cfg(all(target_os = "windows", feature = "runtime-d3d12"))]
            LibrashaderError::D3D12FilterError(_) => LIBRA_ERRNO::RUNTIME_ERROR,
            #[cfg(all(target_os = "windows", feature = "runtime-d3d9"))]
            LibrashaderError::D3D9FilterError(_) => LIBRA_ERRNO::RUNTIME_ERROR,
            #[cfg(feature = "runtime-vulkan")]
            LibrashaderError::VulkanFilterError(_) => LIBRA_ERRNO::RUNTIME_ERROR,
            #[cfg(all(target_vendor = "apple", feature = "runtime-metal"))]
            LibrashaderError::MetalFilterError(_) => LIBRA_ERRNO::RUNTIME_ERROR,
            LibrashaderError::Infallible(_) => LIBRA_ERRNO::UNKNOWN_ERROR,
        }
    }
    pub(crate) const fn ok() -> libra_error_t {
        None
    }

    pub(crate) fn export(self) -> libra_error_t {
        NonNull::new(Box::into_raw(Box::new(self)))
    }
}

macro_rules! assert_non_null {
    (@EXPORT $value:ident) => {
        if $value.is_null() || !$crate::ffi::ptr_is_aligned($value) {
            return $crate::error::LibrashaderError::InvalidParameter(stringify!($value)).export();
        }
    };
    ($value:ident) => {
        if $value.is_null() || !$crate::ffi::ptr_is_aligned($value) {
            return Err($crate::error::LibrashaderError::InvalidParameter(
                stringify!($value),
            ));
        }
    };
}

macro_rules! assert_some_ptr {
    ($value:ident) => {
        if $value.is_none() {
            return Err($crate::error::LibrashaderError::InvalidParameter(
                stringify!($value),
            ));
        }

        let $value = unsafe { $value.as_ref().unwrap_unchecked().as_ref() };
    };
    (mut $value:ident) => {
        if $value.is_none() {
            return Err($crate::error::LibrashaderError::InvalidParameter(
                stringify!($value),
            ));
        }

        let $value = unsafe { $value.as_mut().unwrap_unchecked().as_mut() };
    };
}

use crate::ctypes::libra_error_t;
pub(crate) use assert_non_null;

// pub(crate) use assert_some;
pub(crate) use assert_some_ptr;
