//! librashader preset C API (`libra_preset_*`).
use crate::ctypes::{libra_preset_ctx_t, libra_shader_preset_t};
use crate::error::{assert_non_null, assert_some_ptr, LibrashaderError};
use crate::ffi::extern_fn;
use librashader::presets::ShaderPreset;
use std::ffi::{c_char, CStr, CString};
use std::mem::MaybeUninit;
use std::ptr::NonNull;

const _: () = crate::assert_thread_safe::<ShaderPreset>();

/// A list of preset parameters.
#[repr(C)]
pub struct libra_preset_param_list_t {
    /// A pointer to the parameter
    pub parameters: *const libra_preset_param_t,
    /// The number of parameters in the list.
    pub length: u64,
    /// For internal use only.
    /// Changing this causes immediate undefined behaviour on freeing this parameter list.
    pub _internal_alloc: u64,
}

/// A preset parameter.
#[repr(C)]
pub struct libra_preset_param_t {
    /// The name of the parameter
    pub name: *const c_char,
    /// The description of the parameter.
    pub description: *const c_char,
    /// The initial value the parameter is set to.
    pub initial: f32,
    /// The minimum value that the parameter can be set to.
    pub minimum: f32,
    /// The maximum value that the parameter can be set to.
    pub maximum: f32,
    /// The step by which this parameter can be incremented or decremented.
    pub step: f32,
}

extern_fn! {
    /// Load a preset.
    ///
    /// ## Safety
    ///  - `filename` must be either null or a valid, aligned pointer to a string path to the shader preset.
    ///  - `out` must be either null, or an aligned pointer to an uninitialized or invalid `libra_shader_preset_t`.
    /// ## Returns
    ///  - If any parameters are null, `out` is unchanged, and this function returns `LIBRA_ERR_INVALID_PARAMETER`.
    fn libra_preset_create(
        filename: *const c_char,
        out: *mut MaybeUninit<libra_shader_preset_t>
    ) {
        assert_non_null!(filename);
        assert_non_null!(out);

        let filename = unsafe { CStr::from_ptr(filename) };
        let filename = filename.to_str()?;

        let preset = ShaderPreset::try_parse(filename)?;
        unsafe {
            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                preset,
            )))))
        }
    }
}

extern_fn! {
    /// Load a preset with the given wildcard context.
    ///
    /// The wildcard context is immediately invalidated and must be recreated after
    /// the preset is created.
    ///
    /// Path information variables `PRESET_DIR` and `PRESET` will automatically be filled in.
    /// ## Safety
    ///  - `filename` must be either null or a valid, aligned pointer to a string path to the shader preset.
    ///  - `context` must be either null or a valid, aligned pointer to a initialized `libra_preset_ctx_t`.
    ///  - `context` is  invalidated after this function returns.
    ///  - `out` must be either null, or an aligned pointer to an uninitialized or invalid `libra_shader_preset_t`.
    /// ## Returns
    ///  - If any parameters are null, `out` is unchanged, and this function returns `LIBRA_ERR_INVALID_PARAMETER`.
    fn libra_preset_create_with_context(
        filename: *const c_char,
        context: *mut libra_preset_ctx_t,
        out: *mut MaybeUninit<libra_shader_preset_t>
    ) {
        assert_non_null!(filename);
        assert_non_null!(context);
        assert_non_null!(out);

        let filename = unsafe { CStr::from_ptr(filename) };
        let filename = filename.to_str()?;

        let mut context = unsafe {
            let context_ptr = &mut *context;
            let context = context_ptr.take();
            Box::from_raw(context.unwrap().as_ptr())
        };

        context.add_path_defaults(filename);

        let preset = ShaderPreset::try_parse_with_context(filename, *context)?;
        unsafe {
            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                preset,
            )))))
        }
    }
}

extern_fn! {
    /// Free the preset.
    ///
    /// If `preset` is null, this function does nothing. The resulting value in `preset` then becomes
    /// null.
    ///
    /// ## Safety
    /// - `preset` must be a valid and aligned pointer to a `libra_shader_preset_t`.
    fn libra_preset_free(preset: *mut libra_shader_preset_t) {
        assert_non_null!(preset);
        unsafe {
            let preset_ptr = &mut *preset;
            let preset = preset_ptr.take();
            drop(Box::from_raw(preset.unwrap().as_ptr()));
        }
    }
}

extern_fn! {
    /// Set the value of the parameter in the preset.
    ///
    /// ## Safety
    /// - `preset` must be null or a valid and aligned pointer to a `libra_shader_preset_t`.
    /// - `name` must be null or a valid and aligned pointer to a string.
    fn libra_preset_set_param(
        preset: *mut libra_shader_preset_t,
        name: *const c_char,
        value: f32
    ) |name|; mut |preset| {
        let name = unsafe {
            CStr::from_ptr(name)
        };

        let name = name.to_str()?;
        assert_some_ptr!(mut preset);

        if let Some(param) = preset.parameters.iter_mut().find(|c| c.name == name) {
            param.value = value
        }
    }
}

extern_fn! {
    /// Get the value of the parameter as set in the preset.
    ///
    /// ## Safety
    /// - `preset` must be null or a valid and aligned pointer to a shader preset.
    /// - `name` must be null or a valid and aligned pointer to a string.
    /// - `value` may be a pointer to a uninitialized `float`.
    fn libra_preset_get_param(
        preset: *mut libra_shader_preset_t,
        name: *const c_char,
        value: *mut MaybeUninit<f32>
    ) |name, preset| {
        let name = unsafe { CStr::from_ptr(name) };
        let name = name.to_str()?;
        assert_some_ptr!(preset);
        assert_non_null!(value);

        if let Some(param) = preset.parameters.iter().find(|c| c.name == name) {
            unsafe { value.write(MaybeUninit::new(param.value)) }
        }
    }
}

extern_fn! {
    /// Pretty print the shader preset.
    ///
    /// ## Safety
    /// - `preset` must be null or a valid and aligned pointer to a `libra_shader_preset_t`.
    fn libra_preset_print(preset: *mut libra_shader_preset_t) |preset| {
        assert_some_ptr!(preset);
        println!("{preset:#?}");
    }
}

extern_fn! {
    /// Get a list of runtime parameters.
    ///
    /// ## Safety
    /// - `preset` must be null or a valid and aligned pointer to a `libra_shader_preset_t`.
    /// - `out` must be an aligned pointer to a `libra_preset_parameter_list_t`.
    /// - The output struct should be treated as immutable. Mutating any struct fields
    ///   in the returned struct may at best cause memory leaks, and at worse
    ///   cause undefined behaviour when later freed.
    /// - It is safe to call `libra_preset_get_runtime_params` multiple times, however
    ///   the output struct must only be freed once per call.
    fn libra_preset_get_runtime_params(
        preset: *mut libra_shader_preset_t,
        out: *mut MaybeUninit<libra_preset_param_list_t>
    ) |preset| {
        assert_some_ptr!(preset);
        assert_non_null!(out);

        let iter = librashader::presets::get_parameter_meta(preset)?;
        let mut values = Vec::new();
        for param in iter {
            let name = CString::new(param.id)
            .map_err(|err| LibrashaderError::UnknownError(Box::new(err)))?;
            let description = CString::new(param.description)
            .map_err(|err| LibrashaderError::UnknownError(Box::new(err)))?;
            values.push(libra_preset_param_t {
                name: name.into_raw().cast_const(),
                description: description.into_raw().cast_const(),
                initial: param.initial,
                minimum: param.minimum,
                maximum: param.maximum,
                step: param.step
            })
        }
        let (parts, len, cap) = values.into_raw_parts();
        unsafe {
            out.write(MaybeUninit::new(libra_preset_param_list_t {
                parameters: parts,
                length: len as u64,
                _internal_alloc: cap as u64,
            }));
        }
    }
}

extern_fn! {
    /// Free the runtime parameters.
    ///
    /// Unlike the other `free` functions provided by librashader,
    /// `libra_preset_free_runtime_params` takes the struct directly.
    /// The caller must take care to maintain the lifetime of any pointers
    /// contained within the input `libra_preset_param_list_t`.
    ///
    /// ## Safety
    /// - Any pointers rooted at `parameters` becomes invalid after this function returns,
    ///   including any strings accessible via the input `libra_preset_param_list_t`.
    ///   The caller must ensure that there are no live pointers, aliased or unaliased,
    ///   to data accessible via the input `libra_preset_param_list_t`.
    ///
    /// - Accessing any data pointed to via the input `libra_preset_param_list_t` after it
    ///   has been freed is a use-after-free and is immediate undefined behaviour.
    ///
    /// - If any struct fields of the input `libra_preset_param_list_t` was modified from
    ///   their values given after `libra_preset_get_runtime_params`, this may result
    ///   in undefined behaviour.
    fn libra_preset_free_runtime_params(preset: libra_preset_param_list_t) {
        unsafe {
            let values = Vec::from_raw_parts(preset.parameters.cast_mut(),
                                             preset.length as usize,
                                            preset._internal_alloc as usize);

            for value in values {
                let name = CString::from_raw(value.name.cast_mut());
                let description = CString::from_raw(value.description.cast_mut());

                drop(name);
                drop(description)
            }
        }
    }
}
