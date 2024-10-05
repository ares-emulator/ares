use crate::ctypes::{
    config_struct, libra_gl_filter_chain_t, libra_shader_preset_t, libra_viewport_t, FromUninit,
};
use crate::error::{assert_non_null, assert_some_ptr, LibrashaderError};
use crate::ffi::extern_fn;
use crate::LIBRASHADER_API_VERSION;
use librashader::runtime::gl::{FilterChain, FilterChainOptions, FrameOptions, GLImage};
use librashader::runtime::FilterChainParameters;
use librashader::runtime::{Size, Viewport};
use std::ffi::CStr;
use std::ffi::{c_char, c_void};
use std::mem::MaybeUninit;
use std::num::NonZeroU32;
use std::ptr::NonNull;
use std::slice;
use std::sync::Arc;

/// A GL function loader that librashader needs to be initialized with.
pub type libra_gl_loader_t = unsafe extern "system" fn(*const c_char) -> *const c_void;

/// OpenGL parameters for an image.
#[repr(C)]
pub struct libra_image_gl_t {
    /// A texture GLuint to the texture.
    pub handle: u32,
    /// The format of the texture.
    pub format: u32,
    /// The width of the texture.
    pub width: u32,
    /// The height of the texture.
    pub height: u32,
}

impl From<libra_image_gl_t> for GLImage {
    fn from(value: libra_image_gl_t) -> Self {
        let handle = NonZeroU32::try_from(value.handle)
            .ok()
            .map(glow::NativeTexture);

        GLImage {
            handle,
            format: value.format,
            size: Size::new(value.width, value.height),
        }
    }
}

/// Options for each OpenGL shader frame.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct frame_gl_opt_t {
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
    impl FrameOptions => frame_gl_opt_t {
        0 => [clear_history, frame_direction];
        1 => [rotation, total_subframes, current_subframe]
    }
}

/// Options for filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct filter_chain_gl_opt_t {
    /// The librashader API version.
    pub version: LIBRASHADER_API_VERSION,
    /// The GLSL version. Should be at least `330`.
    pub glsl_version: u16,
    /// Whether or not to use the Direct State Access APIs. Only available on OpenGL 4.5+.
    /// Using the shader cache requires this option, so this option will implicitly
    /// disables the shader cache if false.
    pub use_dsa: bool,
    /// Whether or not to explicitly disable mipmap generation regardless of shader preset settings.
    pub force_no_mipmaps: bool,
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    pub disable_cache: bool,
}

config_struct! {
    impl FilterChainOptions => filter_chain_gl_opt_t {
        0 => [glsl_version, use_dsa, force_no_mipmaps, disable_cache];
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
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    fn libra_gl_filter_chain_create(
        preset: *mut libra_shader_preset_t,
        loader: libra_gl_loader_t,
        options: *const MaybeUninit<filter_chain_gl_opt_t>,
        out: *mut MaybeUninit<libra_gl_filter_chain_t>
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
            let context = glow::Context::from_loader_function_cstr(
                |proc_name| loader(proc_name.as_ptr()));

            let chain = FilterChain::load_from_preset(*preset,
                Arc::new(context), options.as_ref())?;

            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                chain,
            )))))
        }
    }
}

extern_fn! {
    /// Draw a frame with the given parameters for the given filter chain.
    ///
    /// ## Parameters
    ///
    /// - `chain` is a handle to the filter chain.
    /// - `frame_count` is the number of frames passed to the shader
    /// - `image` is a `libra_image_gl_t`, containing the name of a Texture, format, and size information to
    ///    to an image that will serve as the source image for the frame.
    /// - `out` is a `libra_output_framebuffer_gl_t`, containing the name of a Framebuffer, the name of a Texture, format,
    ///    and size information for the render target of the frame.
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
    /// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or invalid, this
    ///    function will return an error.
    /// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16 consecutive `float`
    ///    values for the model view projection matrix.
    /// - `opt` may be null, or if it is not null, must be an aligned pointer to a valid `frame_gl_opt_t`
    ///    struct.
    /// - You must ensure that only one thread has access to `chain` before you call this function. Only one
    ///   thread at a time may call this function. The thread `libra_gl_filter_chain_frame` is called from
    ///   must have its thread-local OpenGL context initialized with the same context used to create
    ///   the filter chain.
    nopanic fn libra_gl_filter_chain_frame(
        chain: *mut libra_gl_filter_chain_t,
        frame_count: usize,
        image: libra_image_gl_t,
        out: libra_image_gl_t,
        viewport: *const libra_viewport_t,
        mvp: *const f32,
        opt: *const MaybeUninit<frame_gl_opt_t>,
    ) mut |chain| {
        assert_some_ptr!(mut chain);
        let image: GLImage = image.into();
        let out: GLImage = out.into();

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
            Viewport::new_render_target_sized_origin(&out, mvp)?
        } else {
            let viewport = unsafe { viewport.read() };
            Viewport {
                x: viewport.x,
                y: viewport.y,
                output: &out,
                size: Size {
                    height: viewport.height,
                    width: viewport.width
                },
                mvp,
            }
        };

        unsafe {
            chain.frame(&image, &viewport, frame_count, opt.as_ref())?;
        }
    }
}

extern_fn! {
    /// Sets a parameter for the filter chain.
    ///
    /// If the parameter does not exist, returns an error.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_gl_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_gl_filter_chain_set_param(
        chain: *mut libra_gl_filter_chain_t,
        param_name: *const c_char,
        value: f32
    ) |chain| {
        assert_some_ptr!(chain);
        assert_non_null!(param_name);
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_gl_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_gl_filter_chain_get_param(
        chain: *const libra_gl_filter_chain_t,
        param_name: *const c_char,
        out: *mut MaybeUninit<f32>
    ) |chain| {
        assert_some_ptr!(chain);
        assert_non_null!(param_name);
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_gl_filter_chain_t`.
    fn libra_gl_filter_chain_set_active_pass_count(
        chain: *mut libra_gl_filter_chain_t,
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_gl_filter_chain_t`.
    fn libra_gl_filter_chain_get_active_pass_count(
        chain: *const libra_gl_filter_chain_t,
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
    /// Free a GL filter chain.
    ///
    /// The resulting value in `chain` then becomes null.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_gl_filter_chain_t`.
    /// - The context that the filter chain was initialized with **must be current** before freeing the filter chain.
    fn libra_gl_filter_chain_free(
        chain: *mut libra_gl_filter_chain_t
    ) {
        assert_non_null!(chain);
        unsafe {
            let chain_ptr = &mut *chain;
            let chain = chain_ptr.take();
            drop(Box::from_raw(chain.unwrap().as_ptr()))
        };
    }
}
