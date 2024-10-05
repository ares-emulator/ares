use crate::ctypes::{
    config_struct, libra_mtl_filter_chain_t, libra_shader_preset_t, libra_viewport_t, FromUninit,
};
use crate::error::{assert_non_null, assert_some_ptr, LibrashaderError};
use crate::ffi::extern_fn;
use librashader::runtime::mtl::{FilterChain, FilterChainOptions, FrameOptions};
use std::ffi::c_char;
use std::ffi::CStr;
use std::mem::MaybeUninit;
use std::ptr::NonNull;
use std::slice;

use librashader::runtime::FilterChainParameters;
use librashader::runtime::{Size, Viewport};

use objc2::runtime::ProtocolObject;
use objc2_metal::{MTLCommandBuffer, MTLCommandQueue, MTLTexture};

use crate::LIBRASHADER_API_VERSION;

/// An alias to a `id<MTLCommandQueue>` protocol object pointer.
pub type PMTLCommandQueue = *const ProtocolObject<dyn MTLCommandQueue>;

/// An alias to a `id<MTLCommandBuffer>` protocol object pointer.
pub type PMTLCommandBuffer = *const ProtocolObject<dyn MTLCommandBuffer>;

/// An alias to a `id<MTLTexture>` protocol object pointer.
pub type PMTLTexture = *const ProtocolObject<dyn MTLTexture>;

/// Options for each Metal shader frame.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct frame_mtl_opt_t {
    /// The librashader API version.
    pub version: LIBRASHADER_API_VERSION,
    /// Whether or not to clear the history buffers.
    pub clear_history: bool,
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    pub frame_direction: i32,
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 3 = 270deg.
    pub rotation: u32,
    /// The total number of subframes ran. Default is 1.
    pub total_subframes: u32,
    /// The current sub frame. Default is 1.
    pub current_subframe: u32,
}

config_struct! {
    impl FrameOptions => frame_mtl_opt_t {
        0 => [clear_history, frame_direction];
        1 => [rotation, total_subframes, current_subframe]
    }
}

/// Options for filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct filter_chain_mtl_opt_t {
    /// The librashader API version.
    pub version: LIBRASHADER_API_VERSION,
    /// Whether or not to explicitly disable mipmap generation regardless of shader preset settings.
    pub force_no_mipmaps: bool,
}

config_struct! {
    impl FilterChainOptions => filter_chain_mtl_opt_t {
        0 => [force_no_mipmaps];
    }
}

extern_fn! {
    /// Create the filter chain given the shader preset.
    ///
    /// The shader preset is immediately invalidated and must be recreated after
    /// the filter chain is created.
    ///
    /// ## Safety:
    /// - `queue` must be valid for the command buffers
    ///     that `libra_mtl_filter_chain_frame` will write to.
    /// - `queue` must be a reference to a `id<MTLCommandQueue>`.
    /// - `preset` must be either null, or valid and aligned.
    /// - `options` must be either null, or valid and aligned.
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    fn libra_mtl_filter_chain_create(
        preset: *mut libra_shader_preset_t,
        queue: PMTLCommandQueue,
        options: *const MaybeUninit<filter_chain_mtl_opt_t>,
        out: *mut MaybeUninit<libra_mtl_filter_chain_t>
    ) |queue| {
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

        let queue = queue.as_ref();
        let options = options.map(FromUninit::from_uninit);

        unsafe {
            let chain = FilterChain::load_from_preset(*preset, queue, options.as_ref())?;

            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                chain,
            )))))
        }
    }
}

extern_fn! {
    /// Create the filter chain given the shader preset deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// The shader preset is immediately invalidated and must be recreated after
    /// the filter chain is created.
    ///
    /// ## Safety:
    /// - `queue` must be valid for the command buffers
    ///     that `libra_mtl_filter_chain_frame` will write to.
    /// - `queue` must be a reference to a `id<MTLCommandQueue>`.
    /// - `command_buffer` must be a valid reference to a `MTLCommandBuffer` that is not already encoding.
    /// - `preset` must be either null, or valid and aligned.
    /// - `options` must be either null, or valid and aligned.
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    ///
    /// The provided command buffer must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling `libra_mtl_filter_chain_frame`.
    fn libra_mtl_filter_chain_create_deferred(
        preset: *mut libra_shader_preset_t,
        queue: PMTLCommandQueue,
        command_buffer: PMTLCommandBuffer,
        options: *const MaybeUninit<filter_chain_mtl_opt_t>,
        out: *mut MaybeUninit<libra_mtl_filter_chain_t>
    ) |queue, command_buffer| {
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
            let chain = FilterChain::load_from_preset_deferred(*preset,
                queue,
                command_buffer,
                options.as_ref())?;

            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                chain,
            )))))
        }
    }
}

extern_fn! {
    /// Records rendering commands for a frame with the given parameters for the given filter chain
    /// to the input command buffer.
    /// ## Parameters
    ///
    /// - `chain` is a handle to the filter chain.
    /// - `command_buffer` is a `MTLCommandBuffer` handle to record draw commands to.
    ///    The provided command buffer must be ready for encoding and contain no prior commands
    /// - `frame_count` is the number of frames passed to the shader
    /// - `image` is a `id<MTLTexture>` that will serve as the source image for the frame.
    /// - `out` is a `id<MTLTexture>` that is the render target of the frame.
    ///
    /// - `viewport` is a pointer to a `libra_viewport_t` that specifies the area onto which scissor and viewport
    ///    will be applied to the render target. It may be null, in which case a default viewport spanning the
    ///    entire render target will be used.
    /// - `mvp` is a pointer to an array of 16 `float` values to specify the model view projection matrix to
    ///    be passed to the shader.
    /// - `options` is a pointer to options for the frame. Valid options are dependent on the `LIBRASHADER_API_VERSION`
    ///    passed in. It may be null, in which case default options for the filter chain are used.
    ///
    /// ## Safety
    /// - `command_buffer` must be a valid reference to a `MTLCommandBuffer` that is not already encoding.
    /// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or invalid, this
    ///    function will return an error.
    /// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16 consecutive `float`
    ///    values for the model view projection matrix.
    /// - `opt` may be null, or if it is not null, must be an aligned pointer to a valid `frame_mtl_opt_t`
    ///    struct.
    /// - You must ensure that only one thread has access to `chain` before you call this function. Only one
    ///   thread at a time may call this function.
    nopanic fn libra_mtl_filter_chain_frame(
        chain: *mut libra_mtl_filter_chain_t,
        command_buffer: PMTLCommandBuffer,
        frame_count: usize,
        image: PMTLTexture,
        output: PMTLTexture,
        viewport: *const libra_viewport_t,
        mvp: *const f32,
        opt: *const MaybeUninit<frame_mtl_opt_t>
    ) |command_buffer, image, output|; mut |chain|  {
        assert_some_ptr!(mut chain);

        let mvp = if mvp.is_null() {
            None
        } else {
            Some(<&[f32; 16]>::try_from(unsafe { slice::from_raw_parts(mvp, 16) }).unwrap())
        };
        let opt = if opt.is_null() {
            None
        } else {
            Some(unsafe { opt.read() })
        };
        let opt = opt.map(FromUninit::from_uninit);

        let viewport = if viewport.is_null() {
            Viewport::new_render_target_sized_origin(output, mvp)?
        } else {
            let viewport = unsafe { viewport.read() };
            Viewport {
                x: viewport.x,
                y: viewport.y,
                output,
                size: Size {
                    height: viewport.height,
                    width: viewport.width
                },
                mvp,
            }
        };

        chain.frame(&image, &viewport, command_buffer, frame_count, opt.as_ref())?;
    }
}

extern_fn! {
    /// Sets a parameter for the filter chain.
    ///
    /// If the parameter does not exist, returns an error.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_mtl_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_mtl_filter_chain_set_param(
        chain: *mut libra_mtl_filter_chain_t,
        param_name: *const c_char,
        value: f32
    ) |chain| {
        assert_some_ptr!(chain);
        unsafe {
            let name = CStr::from_ptr(param_name);
            let name = name.to_str()?;

            if chain.parameters().set_parameter_value(name, value).is_none() {
                return Err(LibrashaderError::UnknownShaderParameter(param_name))
            }
        }
    }
}

extern_fn! {
    /// Gets a parameter for the filter chain.
    ///
    /// If the parameter does not exist, returns an error.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_mtl_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_mtl_filter_chain_get_param(
        chain: *const libra_mtl_filter_chain_t,
        param_name: *const c_char,
        out: *mut MaybeUninit<f32>
    ) |chain| {
        assert_some_ptr!(chain);

        unsafe {
            let name = CStr::from_ptr(param_name);
            let name = name.to_str()?;

            let Some(value) = chain.parameters().parameter_value(name) else {
                return Err(LibrashaderError::UnknownShaderParameter(param_name))
            };

            out.write(MaybeUninit::new(value));
        }
    }
}

extern_fn! {
    /// Sets the number of active passes for this chain.
    ///
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_mtl_filter_chain_t`.
    fn libra_mtl_filter_chain_set_active_pass_count(
        chain: *mut libra_mtl_filter_chain_t,
        value: u32
    ) |chain| {
        assert_some_ptr!(chain);
        chain.parameters().set_passes_enabled(value as usize);
    }
}

extern_fn! {
    /// Gets the number of active passes for this chain.
    ///
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_mtl_filter_chain_t`.
    fn libra_mtl_filter_chain_get_active_pass_count(
        chain: *const libra_mtl_filter_chain_t,
        out: *mut MaybeUninit<u32>
    ) |chain| {
        assert_some_ptr!(chain);
        let value = chain.parameters().passes_enabled();
        unsafe {
            out.write(MaybeUninit::new(value as u32))
        }
    }
}

extern_fn! {
    /// Free a Metal filter chain.
    ///
    /// The resulting value in `chain` then becomes null.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_mtl_filter_chain_t`.
    fn libra_mtl_filter_chain_free(
        chain: *mut libra_mtl_filter_chain_t
    ) {
        assert_non_null!(chain);
        unsafe {
            let chain_ptr = &mut *chain;
            let chain = chain_ptr.take();
            drop(Box::from_raw(chain.unwrap().as_ptr()))
        };
    }
}
