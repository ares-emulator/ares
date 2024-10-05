use crate::ctypes::{
    config_struct, libra_shader_preset_t, libra_viewport_t, libra_vk_filter_chain_t, FromUninit,
};
use crate::error::{assert_non_null, assert_some_ptr, LibrashaderError};
use crate::ffi::extern_fn;
use librashader::runtime::vk::{
    FilterChain, FilterChainOptions, FrameOptions, VulkanImage, VulkanInstance,
};
use std::ffi::c_char;
use std::ffi::CStr;
use std::mem::MaybeUninit;
use std::ptr::NonNull;
use std::slice;

use librashader::runtime::FilterChainParameters;
use librashader::runtime::{Size, Viewport};

use crate::LIBRASHADER_API_VERSION;
use ash::vk;
use ash::vk::Handle;

///	A Vulkan instance function loader that the Vulkan filter chain needs to be initialized with.
pub use ash::vk::PFN_vkGetInstanceProcAddr;

/// Vulkan parameters for an image.
#[repr(C)]
pub struct libra_image_vk_t {
    /// A raw `VkImage` handle.
    pub handle: vk::Image,
    /// The `VkFormat` of the `VkImage`.
    pub format: vk::Format,
    /// The width of the `VkImage`.
    pub width: u32,
    /// The height of the `VkImage`.
    pub height: u32,
}

/// Handles required to instantiate vulkan
#[repr(C)]
pub struct libra_device_vk_t {
    /// A raw `VkPhysicalDevice` handle
    /// for the physical device that will perform rendering.
    pub physical_device: vk::PhysicalDevice,
    /// A raw `VkInstance` handle
    /// for the Vulkan instance that will perform rendering.
    pub instance: vk::Instance,
    /// A raw `VkDevice` handle
    /// for the device attached to the instance that will perform rendering.
    pub device: vk::Device,
    /// The queue to use, if this is `NULL`, then
    /// a suitable queue will be chosen. This must be a graphics queue.
    pub queue: vk::Queue,
    /// The entry loader for the Vulkan library.
    pub entry: Option<vk::PFN_vkGetInstanceProcAddr>,
}

impl From<libra_image_vk_t> for VulkanImage {
    fn from(value: libra_image_vk_t) -> Self {
        VulkanImage {
            size: Size::new(value.width, value.height),
            image: value.handle,
            format: value.format,
        }
    }
}

impl From<libra_device_vk_t> for VulkanInstance {
    fn from(value: libra_device_vk_t) -> Self {
        let queue = if value.queue.is_null() {
            None
        } else {
            Some(value.queue)
        };

        VulkanInstance {
            device: value.device,
            instance: value.instance,
            physical_device: value.physical_device,
            get_instance_proc_addr: value.entry,
            queue,
        }
    }
}

/// Options for each Vulkan shader frame.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct frame_vk_opt_t {
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
    impl FrameOptions => frame_vk_opt_t {
        0 => [clear_history, frame_direction];
        1 => [rotation, total_subframes, current_subframe]
    }
}

/// Options for filter chain creation.
#[repr(C)]
#[derive(Default, Debug, Clone)]
pub struct filter_chain_vk_opt_t {
    /// The librashader API version.
    pub version: LIBRASHADER_API_VERSION,
    /// The number of frames in flight to keep. If zero, defaults to three.
    pub frames_in_flight: u32,
    /// Whether or not to explicitly disable mipmap generation regardless of shader preset settings.
    pub force_no_mipmaps: bool,
    /// Use dynamic rendering over explicit render pass objects.
    /// It is recommended if possible to use dynamic rendering,
    /// because render-pass mode will create new framebuffers per pass.
    pub use_dynamic_rendering: bool,
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    pub disable_cache: bool,
}

config_struct! {
    impl FilterChainOptions => filter_chain_vk_opt_t {
        0 => [frames_in_flight, force_no_mipmaps, use_dynamic_rendering, disable_cache];
    }
}

extern_fn! {
    /// Create the filter chain given the shader preset.
    ///
    /// The shader preset is immediately invalidated and must be recreated after
    /// the filter chain is created.
    ///
    /// ## Safety:
    /// - The handles provided in `vulkan` must be valid for the command buffers that
    ///   `libra_vk_filter_chain_frame` will write to.
    ///    created with the `VK_KHR_dynamic_rendering` extension.
    /// - `preset` must be either null, or valid and aligned.
    /// - `options` must be either null, or valid and aligned.
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    fn libra_vk_filter_chain_create(
        preset: *mut libra_shader_preset_t,
        vulkan: libra_device_vk_t,
        options: *const MaybeUninit<filter_chain_vk_opt_t>,
        out: *mut MaybeUninit<libra_vk_filter_chain_t>
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

        let vulkan: VulkanInstance = vulkan.into();
        let options = options.map(FromUninit::from_uninit);

        unsafe {
            let chain = FilterChain::load_from_preset(*preset, vulkan, options.as_ref())?;

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
    /// - The handles provided in `vulkan` must be valid for the command buffers that
    ///   `libra_vk_filter_chain_frame` will write to.
    /// - `preset` must be either null, or valid and aligned.
    /// - `options` must be either null, or valid and aligned.
    /// - `out` must be aligned, but may be null, invalid, or uninitialized.
    ///
    /// The provided command buffer must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling `libra_vk_filter_chain_frame`.
    fn libra_vk_filter_chain_create_deferred(
        preset: *mut libra_shader_preset_t,
        vulkan: libra_device_vk_t,
        command_buffer: vk::CommandBuffer,
        options: *const MaybeUninit<filter_chain_vk_opt_t>,
        out: *mut MaybeUninit<libra_vk_filter_chain_t>
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

        let vulkan: VulkanInstance = vulkan.into();
        let options = options.map(FromUninit::from_uninit);

        unsafe {
            let chain = FilterChain::load_from_preset_deferred(*preset,
                vulkan,
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
    ///
    /// A pipeline barrier **will not** be created for the final pass. The output image must be
    /// in `VK_COLOR_ATTACHMENT_OPTIMAL`, and will remain so after all shader passes. The caller must transition
    /// the output image to the final layout.
    ///
    /// ## Parameters
    ///
    /// - `chain` is a handle to the filter chain.
    /// - `command_buffer` is a `VkCommandBuffer` handle to record draw commands to.
    ///    The provided command buffer must be ready for recording and contain no prior commands
    /// - `frame_count` is the number of frames passed to the shader
    /// - `image` is a `libra_image_vk_t`, containing a `VkImage` handle, it's format and size information,
    ///    to an image that will serve as the source image for the frame. The input image must be in
    ///    the `VK_SHADER_READ_ONLY_OPTIMAL` layout.
    /// - `out` is a `libra_image_vk_t`, containing a `VkImage` handle, it's format and size information,
    ///    for the render target of the frame. The output image must be in `VK_COLOR_ATTACHMENT_OPTIMAL` layout.
    ///    The output image will remain in `VK_COLOR_ATTACHMENT_OPTIMAL` after all shader passes.
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
    /// - `libra_vk_filter_chain_frame` **must not be called within a RenderPass**.
    /// - `command_buffer` must be a valid handle to a `VkCommandBuffer` that is ready for recording.
    /// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or invalid, this
    ///    function will return an error.
    /// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16 consecutive `float`
    ///    values for the model view projection matrix.
    /// - `opt` may be null, or if it is not null, must be an aligned pointer to a valid `frame_vk_opt_t`
    ///    struct.
    /// - You must ensure that only one thread has access to `chain` before you call this function. Only one
    ///   thread at a time may call this function.
    nopanic fn libra_vk_filter_chain_frame(
        chain: *mut libra_vk_filter_chain_t,
        command_buffer: vk::CommandBuffer,
        frame_count: usize,
        image: libra_image_vk_t,
        out: libra_image_vk_t,
        viewport: *const libra_viewport_t,
        mvp: *const f32,
        opt: *const MaybeUninit<frame_vk_opt_t>
    ) mut |chain| {
        assert_some_ptr!(mut chain);
        let image: VulkanImage = image.into();
        let output = VulkanImage {
            image: out.handle,
            size: Size::new(out.width, out.height),
            format: out.format
        };
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

        unsafe {
            chain.frame(&image, &viewport, command_buffer, frame_count, opt.as_ref())?;
        }
    }
}

extern_fn! {
    /// Sets a parameter for the filter chain.
    ///
    /// If the parameter does not exist, returns an error.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_vk_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_vk_filter_chain_set_param(
        chain: *mut libra_vk_filter_chain_t,
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_vk_filter_chain_t`.
    /// - `param_name` must be either null or a null terminated string.
    fn libra_vk_filter_chain_get_param(
        chain: *const libra_vk_filter_chain_t,
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_vk_filter_chain_t`.
    fn libra_vk_filter_chain_set_active_pass_count(
        chain: *mut libra_vk_filter_chain_t,
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
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_vk_filter_chain_t`.
    fn libra_vk_filter_chain_get_active_pass_count(
        chain: *const libra_vk_filter_chain_t,
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
    /// Free a Vulkan filter chain.
    ///
    /// The resulting value in `chain` then becomes null.
    /// ## Safety
    /// - `chain` must be either null or a valid and aligned pointer to an initialized `libra_vk_filter_chain_t`.
    fn libra_vk_filter_chain_free(
        chain: *mut libra_vk_filter_chain_t
    ) {
        assert_non_null!(chain);
        unsafe {
            let chain_ptr = &mut *chain;
            let chain = chain_ptr.take();
            drop(Box::from_raw(chain.unwrap().as_ptr()))
        };
    }
}
