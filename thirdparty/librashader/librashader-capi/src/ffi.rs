macro_rules! wrap_ok {
    ($e:expr) => {
        ::core::iter::empty().try_fold($e, |_, __x: ::core::convert::Infallible| match __x {})
    };
}

macro_rules! ffi_body {
    (nopanic $body:block) => {
        {
            let result: Result<(), $crate::error::LibrashaderError> = (|| $crate::ffi::wrap_ok!({
                $body
            }))();

            let Err(e) = result else {
                return $crate::error::LibrashaderError::ok()
            };
            e.export()
        }
    };
    ($body:block) => {
        {
            let result = ::std::panic::catch_unwind(::std::panic::AssertUnwindSafe(|| {
                $crate::ffi::ffi_body!(nopanic $body)
            }));

            result.unwrap_or_else(|e| $crate::error::LibrashaderError::UnknownError(e).export())
        }
    };
    (nopanic |$($ref_capture:ident),*|; mut |$($mut_capture:ident),*| $body:block) => {
        {
            $($crate::error::assert_non_null!(@EXPORT $ref_capture);)*
            $(let $ref_capture = unsafe { &*$ref_capture };)*
            $($crate::error::assert_non_null!(@EXPORT $mut_capture);)*
            $(let $mut_capture = unsafe { &mut *$mut_capture };)*
            let result: Result<(), $crate::error::LibrashaderError> = (|| $crate::ffi::wrap_ok!({
                $body
            }))();

            let Err(e) = result else {
                return $crate::error::LibrashaderError::ok()
            };
            e.export()
        }
    };
    (|$($ref_capture:ident),*|; mut |$($mut_capture:ident),*| $body:block) => {
        {
            let result = ::std::panic::catch_unwind(::std::panic::AssertUnwindSafe(|| {
                $crate::ffi::ffi_body!(nopanic |$($ref_capture),*|; mut |$($mut_capture),*| $body)
            }));

            result.unwrap_or_else(|e| $crate::error::LibrashaderError::UnknownError(e).export())
        }
    };
    (nopanic mut |$($mut_capture:ident),*| $body:block) => {
        {
            $($crate::error::assert_non_null!(@EXPORT $mut_capture);)*
            $(let $mut_capture = unsafe { &mut *$mut_capture };)*
            let result: Result<(), $crate::error::LibrashaderError> = (|| $crate::ffi::wrap_ok!({
                $body
            }))();

            let Err(e) = result else {
                return $crate::error::LibrashaderError::ok()
            };
            e.export()
        }
    };
    (mut |$($mut_capture:ident),*| $body:block) => {
         {
            let result = ::std::panic::catch_unwind(::std::panic::AssertUnwindSafe(|| {
                $crate::ffi::ffi_body!(nopanic mut |$($mut_capture),*| $body)
            }));

            result.unwrap_or_else(|e| $crate::error::LibrashaderError::UnknownError(e).export())
        }
    };
    (nopanic |$($ref_capture:ident),*| $body:block) => {
        {
            $($crate::error::assert_non_null!(@EXPORT $ref_capture);)*
            $(let $ref_capture = unsafe { &*$ref_capture };)*
            let result: Result<(), $crate::error::LibrashaderError> = (|| $crate::ffi::wrap_ok!({
                $body
            }))();

            let Err(e) = result else {
                return $crate::error::LibrashaderError::ok()
            };
            e.export()
        }
    };
    (|$($ref_capture:ident),*| $body:block) => {
        {
            let result = ::std::panic::catch_unwind(::std::panic::AssertUnwindSafe(|| {
                $crate::ffi::ffi_body!(nopanic |$($ref_capture),*| $body)
            }));

            result.unwrap_or_else(|e| $crate::error::LibrashaderError::UnknownError(e).export())
        }
    };
}

macro_rules! extern_fn {
    // raw doesn't wrap in ffi_body
    ($(#[$($attrss:tt)*])* raw fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
            #[doc = ::std::stringify!($func_name)]
            pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)* ) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $body
        }
    };

    // ffi_body but panic-safe
    ($(#[$($attrss:tt)*])* fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
            #[doc = ::std::stringify!($func_name)]
            pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!($body)
        }
    };

    ($(#[$($attrss:tt)*])* fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) |$($ref_capture:ident),*|; mut |$($mut_capture:ident),*| $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
            #[doc = ::std::stringify!($func_name)]
            pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!(|$($ref_capture),*|; mut |$($mut_capture),*| $body)
        }
    };

    ($(#[$($attrss:tt)*])* fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) mut |$($mut_capture:ident),*| $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
            #[doc = ::std::stringify!($func_name)]
            pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!(mut |$($mut_capture),*| $body)
        }
    };
    ($(#[$($attrss:tt)*])* fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) |$($ref_capture:ident),*| $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
             #[doc = ::std::stringify!($func_name)]
             pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!(|$($ref_capture),*| $body)
        }
    };

    // nopanic variants that are UB if panics
    ($(#[$($attrss:tt)*])* nopanic fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
            #[doc = ::std::stringify!($func_name)]
            pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!(nopanic $body)
        }
    };

    ($(#[$($attrss:tt)*])* nopanic fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) |$($ref_capture:ident),*|; mut |$($mut_capture:ident),*| $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
            #[doc = ::std::stringify!($func_name)]
            pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!(nopanic |$($ref_capture),*|; mut |$($mut_capture),*| $body)
        }
    };

    ($(#[$($attrss:tt)*])* nopanic fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) mut |$($mut_capture:ident),*| $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
            #[doc = ::std::stringify!($func_name)]
            pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!(nopanic mut |$($mut_capture),*| $body)
        }
    };
    ($(#[$($attrss:tt)*])* nopanic fn $func_name:ident ($($arg_name:ident : $arg_ty:ty),* $(,)?) |$($ref_capture:ident),*| $body:block) => {
        ::paste::paste! {
            /// Function pointer definition for
             #[doc = ::std::stringify!($func_name)]
             pub type [<PFN_ $func_name>] = unsafe extern "C" fn($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t;
        }

        #[no_mangle]
        $(#[$($attrss)*])*
        pub unsafe extern "C" fn $func_name($($arg_name: $arg_ty,)*) -> $crate::ctypes::libra_error_t {
            $crate::ffi::ffi_body!(nopanic |$($ref_capture),*| $body)
        }
    };
}

pub fn boxed_slice_into_raw_parts<T>(vec: Box<[T]>) -> (*mut T, usize) {
    let mut me = ManuallyDrop::new(vec);
    (me.as_mut_ptr(), me.len())
}

pub unsafe fn boxed_slice_from_raw_parts<T>(ptr: *mut T, len: usize) -> Box<[T]> {
    unsafe { Box::from_raw(std::slice::from_raw_parts_mut(ptr, len)) }
}

pub fn ptr_is_aligned<T: Sized>(ptr: *const T) -> bool {
    let align = std::mem::align_of::<T>();
    if !align.is_power_of_two() {
        panic!("is_aligned_to: align is not a power-of-two");
    }
    sptr::Strict::addr(ptr) & (align - 1) == 0
}

pub(crate) use extern_fn;
pub(crate) use ffi_body;
pub(crate) use wrap_ok;

use std::mem::ManuallyDrop;
