//! librashader preset wildcard context C API (`libra_preset_ctx_*`).

use crate::ctypes::{libra_preset_ctx_t, LIBRA_PRESET_CTX_ORIENTATION, LIBRA_PRESET_CTX_RUNTIME};
use crate::error::{assert_non_null, assert_some_ptr};

use librashader::presets::context::{
    ContextItem, PresetExtension, Rotation, ShaderExtension, WildcardContext,
};
use std::ffi::{c_char, CStr};
use std::mem::MaybeUninit;
use std::ptr::NonNull;

use crate::ffi::extern_fn;

const _: () = crate::assert_thread_safe::<WildcardContext>();

extern_fn! {
    /// Create a wildcard context
    ///
    /// The C API does not allow directly setting certain variables
    ///
    /// - `PRESET_DIR` and `PRESET` are inferred on preset creation.
    /// - `VID-DRV-SHADER-EXT` and `VID-DRV-PRESET-EXT` are always set to `slang` and `slangp` for librashader.
    /// - `VID-FINAL-ROT` is automatically calculated as the sum of `VID-USER-ROT` and `CORE-REQ-ROT` if either are present.
    ///
    /// These automatically inferred variables, as well as all other variables can be overridden with
    /// `libra_preset_ctx_set_param`, but the expected string values must be provided.
    /// See <https://github.com/libretro/RetroArch/pull/15023> for a list of expected string values.
    ///
    /// No variables can be removed once added to the context, however subsequent calls to set the same
    /// variable will overwrite the expected variable.
    /// ## Safety
    ///  - `out` must be either null, or an aligned pointer to an uninitialized or invalid `libra_preset_ctx_t`.
    /// ## Returns
    ///  - If any parameters are null, `out` is unchanged, and this function returns `LIBRA_ERR_INVALID_PARAMETER`.
    fn libra_preset_ctx_create(
        out: *mut MaybeUninit<libra_preset_ctx_t>
    ) {
        assert_non_null!(out);

        unsafe {
            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                WildcardContext::new(),
            )))));
        }
    }
}

extern_fn! {
    /// Free the wildcard context.
    ///
    /// If `context` is null, this function does nothing. The resulting value in `context` then becomes
    /// null.
    ///
    /// ## Safety
    /// - `context` must be a valid and aligned pointer to a `libra_preset_ctx_t`
    fn libra_preset_ctx_free(context: *mut libra_preset_ctx_t) {
        assert_non_null!(context);
        unsafe {
            let context_ptr = &mut *context;
            let context = context_ptr.take();
            drop(Box::from_raw(context.unwrap().as_ptr()));
        }
    }
}

extern_fn! {
    /// Set the core name (`CORE`) variable in the context
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    /// - `name` must be null or a valid and aligned pointer to a string.
    fn libra_preset_ctx_set_core_name(
        context: *mut libra_preset_ctx_t,
        name: *const c_char,
    ) |name|; mut |context| {
        let name = unsafe {
            CStr::from_ptr(name)
        };

        let name = name.to_str()?;
        assert_some_ptr!(mut context);

        context.append_item(ContextItem::CoreName(String::from(name)));
    }
}

extern_fn! {
    /// Set the content directory (`CONTENT-DIR`) variable in the context.
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    /// - `name` must be null or a valid and aligned pointer to a string.
    fn libra_preset_ctx_set_content_dir(
        context: *mut libra_preset_ctx_t,
        name: *const c_char,
    ) |name|; mut |context| {
        let name = unsafe {
            CStr::from_ptr(name)
        };

        let name = name.to_str()?;
        assert_some_ptr!(mut context);
        context.append_item(ContextItem::ContentDirectory(String::from(name)));
    }
}

extern_fn! {
    /// Set a custom string variable in context.
    ///
    /// If the path contains this variable when loading a preset, it will be replaced with the
    /// provided contents.
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    /// - `name` must be null or a valid and aligned pointer to a string.
    /// - `value` must be null or a valid and aligned pointer to a string.
    fn libra_preset_ctx_set_param(
        context: *mut libra_preset_ctx_t,
        name: *const c_char,
        value: *const c_char,
    ) |name, value|; mut |context| {
        let name = unsafe {
            CStr::from_ptr(name)
        };
        let name = name.to_str()?;

        let value = unsafe {
            CStr::from_ptr(value)
        };
        let value = value.to_str()?;

        assert_some_ptr!(mut context);
        context.append_item(ContextItem::ExternContext(String::from(name), String::from(value)));
    }
}

extern_fn! {
    /// Set the graphics runtime (`VID-DRV`) variable in the context.
    ///
    /// Note that librashader only supports the following runtimes.
    ///
    /// - Vulkan
    /// - GLCore
    /// - Direct3D11
    /// - Direct3D12
    /// - Metal
    /// - Direct3D9 (HLSL)
    ///
    /// This will also set the appropriate video driver extensions.
    ///
    /// For librashader, `VID-DRV-SHADER-EXT` and `VID-DRV-PRESET-EXT` are always `slang` and `slangp`.
    /// To override this, use `libra_preset_ctx_set_param`.
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    /// - `name` must be null or a valid and aligned pointer to a string.
    fn libra_preset_ctx_set_runtime(
        context: *mut libra_preset_ctx_t,
        value: LIBRA_PRESET_CTX_RUNTIME,
    ) mut |context| {
        assert_some_ptr!(mut context);

        context.append_item(ContextItem::VideoDriverPresetExtension(
            PresetExtension::Slangp,
        ));
        context.append_item(ContextItem::VideoDriverShaderExtension(
            ShaderExtension::Slang,
        ));
        context.append_item(ContextItem::VideoDriver(value.into()));
    }
}

extern_fn! {
    /// Set the core requested rotation (`CORE-REQ-ROT`) variable in the context.
    ///
    /// Rotation is represented by quarter rotations around the unit circle.
    /// For example. `0` = 0deg, `1` = 90deg, `2` = 180deg, `3` = 270deg, `4` = 0deg.
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    fn libra_preset_ctx_set_core_rotation(
        context: *mut libra_preset_ctx_t,
        value: u32,
    ) mut |context| {
        assert_some_ptr!(mut context);
        context.append_item(ContextItem::CoreRequestedRotation(Rotation::from(value)))
    }
}

extern_fn! {
    /// Set the user rotation (`VID-USER-ROT`) variable in the context.
    ///
    /// Rotation is represented by quarter rotations around the unit circle.
    /// For example. `0` = 0deg, `1` = 90deg, `2` = 180deg, `3` = 270deg, `4` = 0deg.
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    fn libra_preset_ctx_set_user_rotation(
        context: *mut libra_preset_ctx_t,
        value: u32,
    ) mut |context| {
        assert_some_ptr!(mut context);
        context.append_item(ContextItem::UserRotation(Rotation::from(value)))
    }
}

extern_fn! {
    /// Set the screen orientation (`SCREEN-ORIENT`) variable in the context.
    ///
    /// Orientation is represented by quarter rotations around the unit circle.
    /// For example. `0` = 0deg, `1` = 90deg, `2` = 180deg, `3` = 270deg, `4` = 0deg.
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    fn libra_preset_ctx_set_screen_orientation(
        context: *mut libra_preset_ctx_t,
        value: u32,
    ) mut |context| {
        assert_some_ptr!(mut context);
        context.append_item(ContextItem::ScreenOrientation(Rotation::from(value)))
    }
}

extern_fn! {
    /// Set whether or not to allow rotation (`VID-ALLOW-CORE-ROT`) variable in the context.
    ///
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    fn libra_preset_ctx_set_allow_rotation(
        context: *mut libra_preset_ctx_t,
        value: bool,
    ) mut |context| {
        assert_some_ptr!(mut context);
        context.append_item(ContextItem::AllowCoreRotation(value.into()))
    }
}

extern_fn! {
    /// Set the view aspect orientation (`VIEW-ASPECT-ORIENT`) variable in the context.
    //////
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    fn libra_preset_ctx_set_view_aspect_orientation(
        context: *mut libra_preset_ctx_t,
        value: LIBRA_PRESET_CTX_ORIENTATION,
    ) mut |context| {
        assert_some_ptr!(mut context);
        context.append_item(ContextItem::ViewAspectOrientation(value.into()))
    }
}

extern_fn! {
    /// Set the core aspect orientation (`CORE-ASPECT-ORIENT`) variable in the context.
    //////
    /// ## Safety
    /// - `context` must be null or a valid and aligned pointer to a `libra_preset_ctx_t`.
    fn libra_preset_ctx_set_core_aspect_orientation(
        context: *mut libra_preset_ctx_t,
        value: LIBRA_PRESET_CTX_ORIENTATION,
    ) mut |context| {
        assert_some_ptr!(mut context);
        context.append_item(ContextItem::CoreAspectOrientation(value.into()))
    }
}
