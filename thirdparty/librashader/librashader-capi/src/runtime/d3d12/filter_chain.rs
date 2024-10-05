use crate::ctypes::{
    config_struct, libra_d3d12_filter_chain_t, libra_shader_preset_t, libra_viewport_t, FromUninit,
};
use crate::error::{assert_non_null, assert_some_ptr, LibrashaderError};
use crate::ffi::extern_fn;
use std::ffi::c_char;
use std::ffi::CStr;
use std::mem::{ManuallyDrop, MaybeUninit};
use std::ptr::NonNull;
use std::slice;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12GraphicsCommandList, ID3D12Resource, D3D12_CPU_DESCRIPTOR_HANDLE,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT;

use crate::LIBRASHADER_API_VERSION;
use librashader::runtime::d3d12::{
    D3D12InputImage, D3D12OutputView, FilterChain, FilterChainOptions, FrameOptions,
};
use librashader::runtime::{FilterChainParameters, Size, Viewport};

/// Tagged union for a Direct3D 12 image
#[repr(C)]
pub struct libra_image_d3d12_t {
    /// The type of the image.
    pub image_type: LIBRA_D3D12_IMAGE_TYPE,
    /// The handle to the image.
    pub handle: libra_image_d3d12_handle_t,
}

/// A handle to a Direct3D 12 image.
///
/// This must be either a pointer to a `ID3D12Resource`,
/// or a valid source or output image type.
#[repr(C)]
pub union libra_image_d3d12_handle_t {
    /// A pointer to an `ID3D12Resource`, with descriptors managed by the filter chain.
    pub resource: ManuallyDrop<ID3D12Resource>,
    /// A source image with externally managed descriptors.
    pub source: ManuallyDrop<libra_source_image_d3d12_t>,
    /// An output image with externally managed descriptors.
    pub output: ManuallyDrop<libra_output_image_d3d12_t>,
}

/// The type of image passed to the image.
#[repr(i32)]
pub enum LIBRA_D3D12_IMAGE_TYPE {
    /// The image handle is a pointer to a `ID3D12Resource`.
    RESOURCE = 0,
    /// The image handle is a `libra_source_image_d3d12_t`
    SOURCE_IMAGE = 1,
    /// The image handle is a `libra_output_image_d3d12_t`
    OUTPUT_IMAGE = 2,
}

/// Direct3D 12 parameters for the source image.
#[repr(C)]
pub struct libra_source_image_d3d12_t {
    /// A CPU descriptor handle to a shader resource view of the image.
    pub descriptor: D3D12_CPU_DESCRIPTOR_HANDLE,
    /// The resource containing the image.
    pub resource: ManuallyDrop<ID3D12Resource>,
}

/// Direct3D 12 parameters for the output image.
#[repr(C)]
pub struct libra_output_image_d3d12_t {
    /// A CPU descriptor handle to a shader resource view of the image.
    pub descriptor: D3D12_CPU_DESCRIPTOR_HANDLE,
    /// The format of the image.
    pub format: DXGI_FORMAT,
    /// The width of the output image.
    pub width: u32,
    /// The height of the output image.
    pub height: u32,
}

/// Options for each Direct3D 12 shader frame.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct frame_d3d12_opt_t {
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
    impl FrameOptions => frame_d3d12_opt_t {
        0 => [clear_history, frame_direction];
        1 => [rotation, total_subframes, current_subframe]
    }
}

/// Options for Direct3D11 filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct filter_chain_d3d12_opt_t {
    /// The librashader API version.
    pub version: LIBRASHADER_API_VERSION,

    /// Force the HLSL shader pipeline. This may reduce shader compatibility.
    pub force_hlsl_pipeline: bool,

    /// Whether or not to explicitly disable mipmap
    /// generation for intermediate passes regardless
    /// of shader preset settings.
    pub force_no_mipmaps: bool,

    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    pub disable_cache: bool,
}

config_struct! {
    impl FilterChainOptions => filter_chain_d3d12_opt_t {
        0 =>  [force_hlsl_pipeline, force_no_mipmaps, disable_cache];
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
    fn libra_d3d12_filter_chain_create(
        preset: *mut libra_shader_preset_t,
        device: ManuallyDrop<ID3D12Device>,
        options: *const MaybeUninit<filter_chain_d3d12_opt_t>,
        out: *mut MaybeUninit<libra_d3d12_filter_chain_t>
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
    /// Create the filter chain given the shader preset deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// The shader preset is immediately invalidated and must be recreated after
    /// the filter chain is created.
    ///
    /// ## Safety:
    /// - `preset` must be either null, or valid and aligned.
    /// - `options` must be either null, or valid and aligned.
    /// - `device` must not be null.
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    /// - `cmd` must not be null.
    ///
    /// The provided command list must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command list and immediately submitting it to a
    /// graphics queue. The command list must be completely executed before calling `libra_d3d12_filter_chain_frame`.
    fn libra_d3d12_filter_chain_create_deferred(
        preset: *mut libra_shader_preset_t,
        device: ManuallyDrop<ID3D12Device>,
        command_list: ManuallyDrop<ID3D12GraphicsCommandList>,
        options: *const MaybeUninit<filter_chain_d3d12_opt_t>,
        out: *mut MaybeUninit<libra_d3d12_filter_chain_t>
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
                &command_list,
                options.as_ref(),
            )?;

            out.write(MaybeUninit::new(NonNull::new(Box::into_raw(Box::new(
                chain,
            )))))
        }
    }
}

extern_fn! {
    /// Records rendering commands for a frame with the given parameters for the given filter chain
    /// to the input command list.
    ///
    /// A resource barrier **will not** be created for the final pass. The output image will
    /// remain in `D3D12_RESOURCE_STATE_RENDER_TARGET` after all shader passes. The caller must transition
    /// the output image to the final resource state.
    ///
    /// The refcount of any COM pointers passed into this frame will not be changed. It is the responsibility
    /// of the caller to ensure any resources remain alive until the `ID3D12GraphicsCommandList` provided is
    /// submitted.
    ///
    /// ## Parameters
    ///
    /// - `chain` is a handle to the filter chain.
    /// - `command_list` is a `ID3D12GraphicsCommandList` to record draw commands to.
    ///    The provided command list must be open and associated with the `ID3D12Device` this filter chain was created with.
    /// - `frame_count` is the number of frames passed to the shader
    /// - `image` is a `libra_image_d3d12_t` with `image_type` set to `IMAGE_TYPE_SOURCE_IMAGE` or `IMAGE_TYPE_RESOURCE`,
    ///    with `image_handle` either a `ID3D12Resource*` or an `libra_source_image_d3d12_t` containing a CPU descriptor to a shader resource view,
    ///    and the image resource the view is of, which will serve as the source image for the frame.
    ///    The input image resource must be in the `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` resource state
    ///    or equivalent barrier layout. The image resource must have dimension `D3D12_RESOURCE_DIMENSION_TEXTURE2D`.
    /// - `out` is a `libra_image_d3d12_t`, with `image_type` set to `IMAGE_TYPE_OUTPUT_IMAGE` or `IMAGE_TYPE_RESOURCE`,
    ///    with `image_handle` being either a `ID3D12Resource*` or an `libra_output_image_d3d12_t`, containing a CPU descriptor handle,
    ///    format, and size information for the render target of the frame. The output image must be in
    ///    `D3D12_RESOURCE_STATE_RENDER_TARGET` resource state or equivalent barrier layout.
    ///    The image resource must have dimension `D3D12_RESOURCE_DIMENSION_TEXTURE2D`.
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
    /// - `opt` may be null, or if it is not null, must be an aligned pointer to a valid `frame_d3d12_opt_t`
    ///    struct.
    /// -  Any resource pointers contained within a `libra_image_d3d12_t` must be non-null.
    /// -  The `handle` field of any `libra_image_d3d12_t` must be valid for it's `image_type`.
    ///      - If `image_type` is `IMAGE_TYPE_RESOURCE`, then `handle` must be `ID3D12Resource *`.
    ///      - If `image_type` is `IMAGE_TYPE_SOURCE`, then `handle` must be `libra_source_image_d3d12_t`.
    ///      - If `image_type` is `IMAGE_TYPE_OUTPUT`, then `handle` must be `libra_output_image_d3d12_t`.
    /// - `command_list` must be a non-null pointer to a `ID3D12GraphicsCommandList` that is open,
    ///    and must be associated with the `ID3D12Device` this filter chain was created with.
    /// - All resource pointers contained within a `libra_image_d3d12_t` must remain valid until the `ID3D12GraphicsCommandList *`
    ///   provided is submitted after the call to this function.
    /// - You must ensure that only one thread has access to `chain` before you call this function. Only one
    ///   thread at a time may call this function.
    nopanic fn libra_d3d12_filter_chain_frame(
        chain: *mut libra_d3d12_filter_chain_t,
        command_list: ManuallyDrop<ID3D12GraphicsCommandList>,
        frame_count: usize,
        image: libra_image_d3d12_t,
        out: libra_image_d3d12_t,
        viewport: *const libra_viewport_t,
        mvp: *const f32,
        options: *const MaybeUninit<frame_d3d12_opt_t>
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

        let options = options.map(FromUninit::from_uninit);

        let output = unsafe {
            match out.image_type {
                LIBRA_D3D12_IMAGE_TYPE::RESOURCE => {
                    let out = out.handle.resource;
                    D3D12OutputView::new_from_resource(
                        out,
                        chain,
                    )?
                }
                LIBRA_D3D12_IMAGE_TYPE::OUTPUT_IMAGE => {
                    let out = out.handle.output;
                    D3D12OutputView::new_from_raw(
                        out.descriptor,
                        Size::new(out.width, out.height),
                        out.format,
                    )
                }
                LIBRA_D3D12_IMAGE_TYPE::SOURCE_IMAGE => {
                    return Err(LibrashaderError::InvalidParameter("out"))
                }
            }
        };

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

        let image = unsafe {
            match image.image_type {
                LIBRA_D3D12_IMAGE_TYPE::RESOURCE => {
                    let image = image.handle.resource;
                    D3D12InputImage::Managed(image)
                }
                 LIBRA_D3D12_IMAGE_TYPE::SOURCE_IMAGE => {
                    let image = ManuallyDrop::into_inner(image.handle.source);
                    D3D12InputImage::External {
                        resource: image.resource,
                        descriptor: image.descriptor,
                    }
                }
                LIBRA_D3D12_IMAGE_TYPE::OUTPUT_IMAGE => {
                    return Err(LibrashaderError::InvalidParameter("image"))
                }
            }
        };

        unsafe {
            chain.frame(&command_list, image, &viewport, frame_count, options.as_ref())?;
        }
    }
}

extern_fn! {
    /// Sets a parameter for the filter chain.
    ///
    /// If the parameter does not exist, returns an error.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d12_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_d3d12_filter_chain_set_param(
        chain: *mut libra_d3d12_filter_chain_t,
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d12_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_d3d12_filter_chain_get_param(
        chain: *const libra_d3d12_filter_chain_t,
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d12_filter_chain_t`.
    fn libra_d3d12_filter_chain_set_active_pass_count(
        chain: *mut libra_d3d12_filter_chain_t,
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d12_filter_chain_t`.
    fn libra_d3d12_filter_chain_get_active_pass_count(
        chain: *const libra_d3d12_filter_chain_t,
        out: *mut MaybeUninit<u32>
    ) |chain| {
        assert_some_ptr!(chain);
        unsafe {
            let value = chain.parameters().passes_enabled();
            out.write(MaybeUninit::new(value as u32))
        }
    }
}

extern_fn! {
    /// Free a D3D12 filter chain.
    ///
    /// The resulting value in `chain` then becomes null.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_d3d12_filter_chain_t`.
    fn libra_d3d12_filter_chain_free(chain: *mut libra_d3d12_filter_chain_t) {
        assert_non_null!(chain);
        unsafe {
            let chain_ptr = &mut *chain;
            let chain = chain_ptr.take();
            drop(Box::from_raw(chain.unwrap().as_ptr()))
        };
    }
}
