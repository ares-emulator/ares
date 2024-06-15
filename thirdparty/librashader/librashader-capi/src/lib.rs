#![feature(doc_cfg)]
//! The C API for [librashader](https://docs.rs/librashader/).
//!
//! The librashader C API is designed to be loaded dynamically via `librashader_ld.h`, but static usage is also
//! possible by linking against `librashader.h` as well as any static libraries used by `librashader`.
//!
//! ## Usage
//! ⚠ Rust consumers should use [librashader](https://docs.rs/librashader/) directly instead. ⚠
//!
//! The librashader C API is designed to be easy to use and safe. Most objects are only accessible behind an opaque pointer.
//! Every allocated object can be freed with a corresponding `free` function **for that specific object type**.
//!
//! Once an object is freed, the input pointer is always set to null. Attempting to free an object that was not
//! allocated from `librashader` or trying to free an object with a wrong `free` function results in
//! **immediate undefined behaviour**.
//!
//! In general, all functions will accept null pointers for all parameters. However, passing a null pointer
//! into any function that requires a non-null pointer will result in the function returning an error with code `INVALID_PARAMETER`.
//!
//! All types that begin with an underscore, such as `_libra_error` or `_shader_preset` are handles that
//! can not be constructed validly, and should always be used with pointer indirection via the corresponding `_t` types.
//!
//! All functions have safety invariants labeled `## Safety` that must be upheld. Failure to uphold these invariants
//! will result in **immediate undefined behaviour**. Generally speaking, all pointers passed to functions must be
//! **aligned** regardless of whether or not they are null.
//!
//! ## Booleans
//! Some option structs take `bool` values.
//! Any booleans passed to librashader **must have a bit pattern equivalent to either `1` or `0`**. Any other value will cause
//! **immediate undefined behaviour**. Using `_Bool` from `stdbool.h` should maintain this invariant.
//!
//! ## Errors
//! The librashader C API provides a robust, reflective error system. Every function returns a `libra_error_t`, which is either
//! a null pointer, or a handle to an opaque allocated error object. If the returned error is null, then the function was successful.
//! Otherwise, error information can be accessed via the `libra_error_` set of APIs. If an error indeed occurs, it may be freed by
//! `libra_error_free`.
//!
//! It is **highly recommended** to check for errors after every call to a librashader API like in the following example.
//!
//! ```c
//! libra_preset_t preset;
//! libra_error_t error = libra.preset_create(
//!         "slang-shaders/crt/crt-lottes.slangp", &preset);
//! if (error != NULL) {
//!    libra.error_print(error);
//!    libra.error_free(&error);
//!    exit(1);
//! }
//! ```
//!
//! There is a case to be made for skipping error checking for `*_filter_chain_frame` due to performance reasons,
//! but only if you are certain that the safety invariants are upheld on each call. Failure to check for errors
//! may result in **undefined behaviour** stemming from failure to uphold safety invariants.
//!
//! ## Thread safety
//!
//! Except for the metal runtime, it is in general, **safe** to create a filter chain instance from a different thread,
//! but drawing filter passes must be synchronized externally. The exception to filter chain creation are in OpenGL,
//! where creating the filter chain instance is safe **if and only if** the thread local OpenGL context is initialized
//! to the same context as the drawing thread, and in Direct3D 11, where filter chain creation is unsafe
//! if the `ID3D11Device` was created with `D3D11_CREATE_DEVICE_SINGLETHREADED`. Metal is entirely thread unsafe.
//!
//! You must ensure that only thread has access to a created filter pass **before** you call `*_frame`. `*_frame` may only be
//! called from one thread at a time.

#![allow(non_camel_case_types)]
#![feature(try_blocks)]
#![deny(unsafe_op_in_unsafe_fn)]

extern crate alloc;

pub mod ctypes;
pub mod error;
mod ffi;
pub mod presets;

#[cfg(feature = "reflect")]
#[doc(hidden)]
pub mod reflect;

pub mod runtime;
pub mod version;
pub mod wildcard;

pub use version::LIBRASHADER_ABI_VERSION;
pub use version::LIBRASHADER_API_VERSION;

#[allow(dead_code)]
const fn assert_thread_safe<T: Send + Sync>() {}
