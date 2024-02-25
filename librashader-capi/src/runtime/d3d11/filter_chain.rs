use crate::ctypes::{
    config_struct, libra_d3d11_filter_chain_t, libra_shader_preset_t, libra_viewport_t, FromUninit,
};
use crate::error::{assert_non_null, assert_some_ptr, LibrashaderError};
use crate::ffi::extern_fn;
use librashader::runtime::d3d11::{
    D3D11InputView, D3D11OutputView, FilterChain, FilterChainOptions, FrameOptions,
};
use std::ffi::c_char;
use std::ffi::CStr;
use std::mem::{ManuallyDrop, MaybeUninit};
use std::ptr::NonNull;
use std::slice;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11Device, ID3D11DeviceContext, ID3D11RenderTargetView, ID3D11ShaderResourceView,
};

use crate::LIBRASHADER_API_VERSION;
use librashader::runtime::{FilterChainParameters, Size, Viewport};

/// Direct3D 11 parameters for the source image.
#[repr(C)]
pub struct libra_source_image_d3d11_t {
    /// A shader resource view into the source image
    pub handle: ManuallyDrop<ID3D11ShaderResourceView>,
    /// The width of the source image.
    pub width: u32,
    /// The height of the source image.
    pub height: u32,
}

impl TryFrom<libra_source_image_d3d11_t> for D3D11InputView {
    type Error = LibrashaderError;

    fn try_from(value: libra_source_image_d3d11_t) -> Result<Self, Self::Error> {
        let handle = value.handle.clone();

        Ok(D3D11InputView {
            handle: ManuallyDrop::into_inner(handle),
            size: Size::new(value.width, value.height),
        })
    }
}

/// Options for Direct3D 11 filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct filter_chain_d3d11_opt_t {
    /// The librashader API version.
    pub version: LIBRASHADER_API_VERSION,
    /// Whether or not to explicitly disable mipmap
    /// generation regardless of shader preset settings.
    pub force_no_mipmaps: bool,
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    pub disable_cache: bool,
}

config_struct! {
    impl FilterChainOptions => filter_chain_d3d11_opt_t {
        0 => [force_no_mipmaps, disable_cache];
    }
}

/// Options for each Direct3D 11 shader frame.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct frame_d3d11_opt_t {
    /// The librashader API version.
    pub version: LIBRASHADER_API_VERSION,
    /// Whether or not to clear the history buffers.
    pub clear_history: bool,
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    pub frame_direction: i32,
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 4 = 270deg.
    pub rotation: u32,
    /// The total number of subframes ran. Default is 1.
    pub total_subframes: u32,
    /// The current sub frame. Default is 1.
    pub current_subframe: u32,
}

config_struct! {
    impl FrameOptions => frame_d3d11_opt_t {
        0 => [clear_history, frame_direction];
        1 => [rotation, total_subframes, current_subframe]
    }
}

extern_fn! {
    /// Create the filter chain given the shader preset.
    ///
    /// The shader preset is immediately invalidated and must be recreated after
    /// the filter chain is created.
    ///
    /// ## Safety:
    /// - `preset` must be either null, or valid and aligned.
    /// - `options` must be either null, or valid and aligned.
    /// - `device` must not be null.
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    fn libra_d3d11_filter_chain_create(
        preset: *mut libra_shader_preset_t,
        device: ManuallyDrop<ID3D11Device>,
        options: *const MaybeUninit<filter_chain_d3d11_opt_t>,
        out: *mut MaybeUninit<libra_d3d11_filter_chain_t>
    ) {
        assert_non_null!(preset);
        let preset = unsafe {
            let preset_ptr = &mut *preset;
            let preset = preset_ptr.take();
            Box::from_raw(preset.unwrap().as_ptr())
        };

        let options = if options.is_null() {
            None
        } else {
            Some(unsafe { options.read() })
        };

        let options = options.map(FromUninit::from_uninit);
        unsafe {
            let chain = FilterChain::load_from_preset(
                *preset,
                &device,
                options.as_ref(),
            )?;

            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                chain,
            )))))
        }
    }
}

extern_fn! {
    /// Create the filter chain given the shader preset, deferring and GPU-side initialization
    /// to the caller. This function is therefore requires no external synchronization of the
    /// immediate context, as long as the immediate context is not used as the input context,
    /// nor of the device, as long as the device is not single-threaded only.
    ///
    /// The shader preset is immediately invalidated and must be recreated after
    /// the filter chain is created.
    ///
    /// ## Safety:
    /// - `preset` must be either null, or valid and aligned.
    /// - `options` must be either null, or valid and aligned.
    /// - `device` must not be null.
    /// - `device_context` not be null.
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    ///
    /// The provided context must either be immediate, or immediately submitted after this function
    /// returns, **before drawing frames**, or lookup textures will fail to load and the filter chain
    /// will be in an invalid state.
    ///
    /// If the context is deferred, it must be ready for command recording, and have no prior commands
    /// recorded. No commands shall be recorded after, the caller must immediately call [`FinishCommandList`](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-finishcommandlist)
    /// and execute the command list on the immediate context after this function returns.
    ///
    /// If the context is immediate, then access to the immediate context requires external synchronization.
    fn libra_d3d11_filter_chain_create_deferred(
        preset: *mut libra_shader_preset_t,
        device: ManuallyDrop<ID3D11Device>,
        device_context: ManuallyDrop<ID3D11DeviceContext>,
        options: *const MaybeUninit<filter_chain_d3d11_opt_t>,
        out: *mut MaybeUninit<libra_d3d11_filter_chain_t>
    ) {
        assert_non_null!(preset);
        let preset = unsafe {
            let preset_ptr = &mut *preset;
            let preset = preset_ptr.take();
            Box::from_raw(preset.unwrap().as_ptr())
        };

        let options = if options.is_null() {
            None
        } else {
            Some(unsafe { options.read() })
        };

        let options = options.map(FromUninit::from_uninit);
        unsafe {
            let chain = FilterChain::load_from_preset_deferred(
                *preset,
                &device,
                &device_context,
                options.as_ref(),
            )?;

             out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                chain,
            )))))
        }
    }
}

// This assert ensures that the bindings of Option<ManuallyDrop<ID3D11DeviceContext>> to ID3D11DeviceContext* is sound.
const _: () = assert!(
    std::mem::size_of::<ID3D11DeviceContext>()
        == std::mem::size_of::<Option<ManuallyDrop<ID3D11DeviceContext>>>()
);

extern_fn! {
    /// Draw a frame with the given parameters for the given filter chain.
    ///
    /// If `device_context` is null, then commands are recorded onto the immediate context. Otherwise,
    /// it will record commands onto the provided context. If the context is deferred, librashader
    /// will not finalize command lists. The context must otherwise be associated with the `ID3D11Device`
    //  the filter chain was created with.
    ///
    /// ## Safety
    /// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or invalid, this
    ///    function will return an error.
    /// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16 consecutive `float`
    ///    values for the model view projection matrix.
    /// - `opt` may be null, or if it is not null, must be an aligned pointer to a valid `frame_d3d11_opt_t`
    ///    struct.
    /// - `out` must not be null.
    /// - `image.handle` must not be null.
    /// - If `device_context` is null, commands will be recorded onto the immediate context of the `ID3D11Device`
    ///   this filter chain was created with. The context must otherwise be associated with the `ID3D11Device`
    ///   the filter chain was created with.
    /// - You must ensure that only one thread has access to `chain` before you call this function. Only one
    ///   thread at a time may call this function.
    nopanic fn libra_d3d11_filter_chain_frame(
        chain: *mut libra_d3d11_filter_chain_t,
        // cbindgen can't discover that ID3D11DeviceContext has the niche optimization
        // so ManuallyDrop<Option<ID3D11DeviceContext>> doesn't generate correct bindings.
        device_context: Option<ManuallyDrop<ID3D11DeviceContext>>,
        frame_count: usize,
        image: libra_source_image_d3d11_t,
        viewport: libra_viewport_t,
        out: ManuallyDrop<ID3D11RenderTargetView>,
        mvp: *const f32,
        options: *const MaybeUninit<frame_d3d11_opt_t>
    ) mut |chain| {
        assert_some_ptr!(mut chain);

        let mvp = if mvp.is_null() {
            None
        } else {
            Some(<&[f32; 16]>::try_from(unsafe { slice::from_raw_parts(mvp, 16) }).unwrap())
        };

        let options = if options.is_null() {
            None
        } else {
            Some(unsafe { options.read() })
        };

        let viewport = Viewport {
            x: viewport.x,
            y: viewport.y,
            output: D3D11OutputView {
                size: Size::new(viewport.width, viewport.height),
                handle: ManuallyDrop::into_inner(out.clone()),
            },
            mvp,
        };

        let options = options.map(FromUninit::from_uninit);

        let image = image.try_into()?;

        unsafe {
            chain.frame(device_context.as_deref(), image, &viewport, frame_count, options.as_ref())?;
        }
    }
}

extern_fn! {
    /// Sets a parameter for the filter chain.
    ///
    /// If the parameter does not exist, returns an error.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d11_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_d3d11_filter_chain_set_param(
        chain: *mut libra_d3d11_filter_chain_t,
        param_name: *const c_char,
        value: f32
    ) mut |chain| {
        assert_some_ptr!(mut chain);
        assert_non_null!(param_name);
        unsafe {
            let name = CStr::from_ptr(param_name);
            let name = name.to_str()?;

            if chain.set_parameter(name, value).is_none() {
                return LibrashaderError::UnknownShaderParameter(param_name).export()
            }
        }
    }
}

extern_fn! {
    /// Gets a parameter for the filter chain.
    ///
    /// If the parameter does not exist, returns an error.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d11_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_d3d11_filter_chain_get_param(
        chain: *mut libra_d3d11_filter_chain_t,
        param_name: *const c_char,
        out: *mut MaybeUninit<f32>
    ) mut |chain| {
        assert_some_ptr!(mut chain);
        assert_non_null!(param_name);
        unsafe {
            let name = CStr::from_ptr(param_name);
            let name = name.to_str()?;

            let Some(value) = chain.get_parameter(name) else {
                return LibrashaderError::UnknownShaderParameter(param_name).export()
            };

            out.write(MaybeUninit::new(value));
        }
    }
}

extern_fn! {
    /// Sets the number of active passes for this chain.
    ///
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d11_filter_chain_t`.
    fn libra_d3d11_filter_chain_set_active_pass_count(
        chain: *mut libra_d3d11_filter_chain_t,
        value: u32
    ) mut |chain| {
        assert_some_ptr!(mut chain);
        chain.set_enabled_pass_count(value as usize);
    }
}

extern_fn! {
    /// Gets the number of active passes for this chain.
    ///
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d11_filter_chain_t`.
    fn libra_d3d11_filter_chain_get_active_pass_count(
        chain: *mut libra_d3d11_filter_chain_t,
        out: *mut MaybeUninit<u32>
    ) mut |chain| {
        assert_some_ptr!(mut chain);
        unsafe {
            let value = chain.get_enabled_pass_count();
            out.write(MaybeUninit::new(value as u32))
        }
    }
}

extern_fn! {
    /// Free a D3D11 filter chain.
    ///
    /// The resulting value in `chain` then becomes null.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d11_filter_chain_t`.
    fn libra_d3d11_filter_chain_free(chain: *mut libra_d3d11_filter_chain_t) {
        assert_non_null!(chain);
        unsafe {
            let chain_ptr = &mut *chain;
            let chain = chain_ptr.take();
            drop(Box::from_raw(chain.unwrap().as_ptr()))
        };
    }
}
