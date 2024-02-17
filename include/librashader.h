/*
librashader.h
SPDX-License-Identifier: MIT
This file is part of the librashader C headers.

Copyright 2022 chyyran

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __LIBRASHADER_H__
#define __LIBRASHADER_H__

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#if defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11)
#include <d3d11.h>
#endif
#if defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12)
#include <d3d12.h>
#endif
#if defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__)
#import <Metal/Metal.h>
#endif
#if defined(LIBRA_RUNTIME_VULKAN)
#include <vulkan\vulkan.h>
#endif

/// Error codes for librashader error types.
enum LIBRA_ERRNO
#ifdef __cplusplus
    : int32_t
#endif    // __cplusplus
{
    LIBRA_ERRNO_UNKNOWN_ERROR = 0,
    LIBRA_ERRNO_INVALID_PARAMETER = 1,
    LIBRA_ERRNO_INVALID_STRING = 2,
    LIBRA_ERRNO_PRESET_ERROR = 3,
    LIBRA_ERRNO_PREPROCESS_ERROR = 4,
    LIBRA_ERRNO_SHADER_PARAMETER_ERROR = 5,
    LIBRA_ERRNO_REFLECT_ERROR = 6,
    LIBRA_ERRNO_RUNTIME_ERROR = 7,
};
#ifndef __cplusplus
typedef int32_t LIBRA_ERRNO;
#endif    // __cplusplus

/// An enum representing orientation for use in preset contexts.
enum LIBRA_PRESET_CTX_ORIENTATION
#ifdef __cplusplus
    : uint32_t
#endif    // __cplusplus
{
    LIBRA_PRESET_CTX_ORIENTATION_VERTICAL = 0,
    LIBRA_PRESET_CTX_ORIENTATION_HORIZONTAL,
};
#ifndef __cplusplus
typedef uint32_t LIBRA_PRESET_CTX_ORIENTATION;
#endif    // __cplusplus

enum LIBRA_PRESET_CTX_RUNTIME
#ifdef __cplusplus
    : uint32_t
#endif    // __cplusplus
{
    LIBRA_PRESET_CTX_RUNTIME_NONE = 0,
    LIBRA_PRESET_CTX_RUNTIME_GL_CORE,
    LIBRA_PRESET_CTX_RUNTIME_VULKAN,
    LIBRA_PRESET_CTX_RUNTIME_D3D11,
    LIBRA_PRESET_CTX_RUNTIME_D3D12,
    LIBRA_PRESET_CTX_RUNTIME_METAL,
};
#ifndef __cplusplus
typedef uint32_t LIBRA_PRESET_CTX_RUNTIME;
#endif    // __cplusplus

/// Opaque struct for a Direct3D 11 filter chain.
typedef struct _filter_chain_d3d11 _filter_chain_d3d11;

/// Opaque struct for a Direct3D 12 filter chain.
typedef struct _filter_chain_d3d12 _filter_chain_d3d12;

/// Opaque struct for an OpenGL filter chain.
typedef struct _filter_chain_gl _filter_chain_gl;

/// Opaque struct for a Metal filter chain.
typedef struct _filter_chain_mtl _filter_chain_mtl;

/// Opaque struct for a Vulkan filter chain.
typedef struct _filter_chain_vk _filter_chain_vk;

/// The error type for librashader C API.
typedef struct _libra_error _libra_error;

/// Opaque struct for a shader preset.
typedef struct _shader_preset _shader_preset;

/// Opaque struct for a preset context.
typedef struct _preset_ctx _preset_ctx;

/// A handle to a librashader error object.
typedef struct _libra_error *libra_error_t;

/// A handle to a shader preset object.
typedef struct _shader_preset *libra_shader_preset_t;

/// A handle to a preset wildcard context object.
typedef struct _preset_ctx *libra_preset_ctx_t;

/// A preset parameter.
typedef struct libra_preset_param_t {
    /// The name of the parameter
    const char *name;
    /// The description of the parameter.
    const char *description;
    /// The initial value the parameter is set to.
    float initial;
    /// The minimum value that the parameter can be set to.
    float minimum;
    /// The maximum value that the parameter can be set to.
    float maximum;
    /// The step by which this parameter can be incremented or decremented.
    float step;
} libra_preset_param_t;

/// A list of preset parameters.
typedef struct libra_preset_param_list_t {
    /// A pointer to the parameter
    const struct libra_preset_param_t *parameters;
    /// The number of parameters in the list.
    uint64_t length;
    /// For internal use only.
    /// Changing this causes immediate undefined behaviour on freeing this
    /// parameter list.
    uint64_t _internal_alloc;
} libra_preset_param_list_t;

#if defined(LIBRA_RUNTIME_OPENGL)
/// A GL function loader that librashader needs to be initialized with.
typedef const void *(*libra_gl_loader_t)(const char *);
#endif

/// API version type alias.
typedef size_t LIBRASHADER_API_VERSION;

#if defined(LIBRA_RUNTIME_OPENGL)
/// Options for filter chain creation.
typedef struct filter_chain_gl_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// The GLSL version. Should be at least `330`.
    uint16_t glsl_version;
    /// Whether or not to use the Direct State Access APIs. Only available on
    /// OpenGL 4.5+. Using the shader cache requires this option, so this option
    /// will implicitly disables the shader cache if false.
    bool use_dsa;
    /// Whether or not to explicitly disable mipmap generation regardless of
    /// shader preset settings.
    bool force_no_mipmaps;
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    bool disable_cache;
} filter_chain_gl_opt_t;
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// A handle to a OpenGL filter chain.
typedef struct _filter_chain_gl *libra_gl_filter_chain_t;
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// OpenGL parameters for the source image.
typedef struct libra_source_image_gl_t {
    /// A texture GLuint to the source image.
    uint32_t handle;
    /// The format of the source image.
    uint32_t format;
    /// The width of the source image.
    uint32_t width;
    /// The height of the source image.
    uint32_t height;
} libra_source_image_gl_t;
#endif

/// Defines the output viewport for a rendered frame.
typedef struct libra_viewport_t {
    /// The x offset in the viewport framebuffer to begin rendering from.
    float x;
    /// The y offset in the viewport framebuffer to begin rendering from.
    float y;
    /// The width of the viewport framebuffer.
    uint32_t width;
    /// The height of the viewport framebuffer.
    uint32_t height;
} libra_viewport_t;

#if defined(LIBRA_RUNTIME_OPENGL)
/// OpenGL parameters for the output framebuffer.
typedef struct libra_output_framebuffer_gl_t {
    /// A framebuffer GLuint to the output framebuffer.
    uint32_t fbo;
    /// A texture GLuint to the logical buffer of the output framebuffer.
    uint32_t texture;
    /// The format of the output framebuffer.
    uint32_t format;
} libra_output_framebuffer_gl_t;
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Options for each OpenGL shader frame.
typedef struct frame_gl_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Whether or not to clear the history buffers.
    bool clear_history;
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    int32_t frame_direction;
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 4 = 270deg.
    uint32_t rotation;
    /// The total number of subframes ran. Default is 1.
    uint32_t total_subframes;
    /// The current sub frame. Default is 1.
    uint32_t current_subframe;
} frame_gl_opt_t;
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Handles required to instantiate vulkan
typedef struct libra_device_vk_t {
    /// A raw `VkPhysicalDevice` handle
    /// for the physical device that will perform rendering.
    VkPhysicalDevice physical_device;
    /// A raw `VkInstance` handle
    /// for the Vulkan instance that will perform rendering.
    VkInstance instance;
    /// A raw `VkDevice` handle
    /// for the device attached to the instance that will perform rendering.
    VkDevice device;
    /// The entry loader for the Vulkan library.
    PFN_vkGetInstanceProcAddr entry;
} libra_device_vk_t;
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Options for filter chain creation.
typedef struct filter_chain_vk_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// The number of frames in flight to keep. If zero, defaults to three.
    uint32_t frames_in_flight;
    /// Whether or not to explicitly disable mipmap generation regardless of
    /// shader preset settings.
    bool force_no_mipmaps;
    /// Use dynamic rendering over explicit render pass objects.
    /// It is recommended if possible to use dynamic rendering,
    /// because render-pass mode will create new framebuffers per pass.
    bool use_dynamic_rendering;
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    bool disable_cache;
} filter_chain_vk_opt_t;
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// A handle to a Vulkan filter chain.
typedef struct _filter_chain_vk *libra_vk_filter_chain_t;
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Vulkan parameters for the source image.
typedef struct libra_source_image_vk_t {
    /// A raw `VkImage` handle to the source image.
    VkImage handle;
    /// The `VkFormat` of the source image.
    VkFormat format;
    /// The width of the source image.
    uint32_t width;
    /// The height of the source image.
    uint32_t height;
} libra_source_image_vk_t;
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Vulkan parameters for the output image.
typedef struct libra_output_image_vk_t {
    /// A raw `VkImage` handle to the output image.
    VkImage handle;
    /// The `VkFormat` of the output image.
    VkFormat format;
} libra_output_image_vk_t;
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Options for each Vulkan shader frame.
typedef struct frame_vk_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Whether or not to clear the history buffers.
    bool clear_history;
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    int32_t frame_direction;
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 4 = 270deg.
    uint32_t rotation;
    /// The total number of subframes ran. Default is 1.
    uint32_t total_subframes;
    /// The current sub frame. Default is 1.
    uint32_t current_subframe;
} frame_vk_opt_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Options for Direct3D 11 filter chain creation.
typedef struct filter_chain_d3d11_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Whether or not to explicitly disable mipmap
    /// generation regardless of shader preset settings.
    bool force_no_mipmaps;
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    bool disable_cache;
} filter_chain_d3d11_opt_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// A handle to a Direct3D 11 filter chain.
typedef struct _filter_chain_d3d11 *libra_d3d11_filter_chain_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Direct3D 11 parameters for the source image.
typedef struct libra_source_image_d3d11_t {
    /// A shader resource view into the source image
    ID3D11ShaderResourceView *handle;
    /// The width of the source image.
    uint32_t width;
    /// The height of the source image.
    uint32_t height;
} libra_source_image_d3d11_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Options for each Direct3D 11 shader frame.
typedef struct frame_d3d11_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Whether or not to clear the history buffers.
    bool clear_history;
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    int32_t frame_direction;
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 4 = 270deg.
    uint32_t rotation;
    /// The total number of subframes ran. Default is 1.
    uint32_t total_subframes;
    /// The current sub frame. Default is 1.
    uint32_t current_subframe;
} frame_d3d11_opt_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Options for Direct3D11 filter chain creation.
typedef struct filter_chain_d3d12_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Force the HLSL shader pipeline. This may reduce shader compatibility.
    bool force_hlsl_pipeline;
    /// Whether or not to explicitly disable mipmap
    /// generation for intermediate passes regardless
    /// of shader preset settings.
    bool force_no_mipmaps;
    /// Disable the shader object cache. Shaders will be
    /// recompiled rather than loaded from the cache.
    bool disable_cache;
} filter_chain_d3d12_opt_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// A handle to a Direct3D 12 filter chain.
typedef struct _filter_chain_d3d12 *libra_d3d12_filter_chain_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Direct3D 12 parameters for the source image.
typedef struct libra_source_image_d3d12_t {
    /// The resource containing the image.
    ID3D12Resource *resource;
    /// A CPU descriptor handle to a shader resource view of the image.
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
    /// The format of the image.
    DXGI_FORMAT format;
    /// The width of the source image.
    uint32_t width;
    /// The height of the source image.
    uint32_t height;
} libra_source_image_d3d12_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Direct3D 12 parameters for the output image.
typedef struct libra_output_image_d3d12_t {
    /// A CPU descriptor handle to a shader resource view of the image.
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
    /// The format of the image.
    DXGI_FORMAT format;
} libra_output_image_d3d12_t;
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Options for each Direct3D 12 shader frame.
typedef struct frame_d3d12_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Whether or not to clear the history buffers.
    bool clear_history;
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    int32_t frame_direction;
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 4 = 270deg.
    uint32_t rotation;
    /// The total number of subframes ran. Default is 1.
    uint32_t total_subframes;
    /// The current sub frame. Default is 1.
    uint32_t current_subframe;
} frame_d3d12_opt_t;
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Options for filter chain creation.
typedef struct filter_chain_mtl_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Whether or not to explicitly disable mipmap generation regardless of
    /// shader preset settings.
    bool force_no_mipmaps;
} filter_chain_mtl_opt_t;
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
typedef struct _filter_chain_mtl *libra_mtl_filter_chain_t;
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Options for each Metal shader frame.
typedef struct frame_mtl_opt_t {
    /// The librashader API version.
    LIBRASHADER_API_VERSION version;
    /// Whether or not to clear the history buffers.
    bool clear_history;
    /// The direction of rendering.
    /// -1 indicates that the frames are played in reverse order.
    int32_t frame_direction;
    /// The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 4 = 270deg.
    uint32_t rotation;
    /// The total number of subframes ran. Default is 1.
    uint32_t total_subframes;
    /// The current sub frame. Default is 1.
    uint32_t current_subframe;
} frame_mtl_opt_t;
#endif

typedef size_t LIBRASHADER_ABI_VERSION;

/// Function pointer definition for libra_abi_version
typedef LIBRASHADER_ABI_VERSION (*PFN_libra_instance_abi_version)(void);

/// Function pointer definition for libra_abi_version
typedef LIBRASHADER_API_VERSION (*PFN_libra_instance_api_version)(void);

/// Function pointer definition for
/// libra_preset_create
typedef libra_error_t (*PFN_libra_preset_create)(const char *filename,
                                                 libra_shader_preset_t *out);

/// Function pointer definition for
/// libra_preset_free
typedef libra_error_t (*PFN_libra_preset_free)(libra_shader_preset_t *preset);

/// Function pointer definition for
/// libra_preset_set_param
typedef libra_error_t (*PFN_libra_preset_set_param)(
    libra_shader_preset_t *preset, const char *name, float value);

/// Function pointer definition for
/// libra_preset_get_param
typedef libra_error_t (*PFN_libra_preset_get_param)(
    libra_shader_preset_t *preset, const char *name, float *value);

/// Function pointer definition for
/// libra_preset_print
typedef libra_error_t (*PFN_libra_preset_print)(libra_shader_preset_t *preset);

/// Function pointer definition for
/// libra_preset_get_runtime_params
typedef libra_error_t (*PFN_libra_preset_get_runtime_params)(
    libra_shader_preset_t *preset, struct libra_preset_param_list_t *out);

/// Function pointer definition for
/// libra_preset_free_runtime_params
typedef libra_error_t (*PFN_libra_preset_free_runtime_params)(
    struct libra_preset_param_list_t preset);

/// Function pointer definition for
/// libra_preset_create_with_context
typedef libra_error_t (*PFN_libra_preset_create_with_context)(
    const char *filename, libra_preset_ctx_t *context,
    libra_shader_preset_t *out);

/// Function pointer definition for
/// libra_preset_ctx_create
typedef libra_error_t (*PFN_libra_preset_ctx_create)(libra_preset_ctx_t *out);

/// Function pointer definition for
/// libra_preset_ctx_free
typedef libra_error_t (*PFN_libra_preset_ctx_free)(libra_preset_ctx_t *context);

/// Function pointer definition for
/// libra_preset_ctx_set_core_name
typedef libra_error_t (*PFN_libra_preset_ctx_set_core_name)(
    libra_preset_ctx_t *context, const char *name);

/// Function pointer definition for
/// libra_preset_ctx_set_content_dir
typedef libra_error_t (*PFN_libra_preset_ctx_set_content_dir)(
    libra_preset_ctx_t *context, const char *name);

/// Function pointer definition for
/// libra_preset_ctx_set_param
typedef libra_error_t (*PFN_libra_preset_ctx_set_param)(
    libra_preset_ctx_t *context, const char *name, const char *value);

/// Function pointer definition for
/// libra_preset_ctx_set_core_rotation
typedef libra_error_t (*PFN_libra_preset_ctx_set_core_rotation)(
    libra_preset_ctx_t *context, uint32_t value);

/// Function pointer definition for
/// libra_preset_ctx_set_user_rotation
typedef libra_error_t (*PFN_libra_preset_ctx_set_user_rotation)(
    libra_preset_ctx_t *context, uint32_t value);

/// Function pointer definition for
/// libra_preset_ctx_set_screen_orientation
typedef libra_error_t (*PFN_libra_preset_ctx_set_screen_orientation)(
    libra_preset_ctx_t *context, uint32_t value);

/// Function pointer definition for
/// libra_preset_ctx_set_allow_rotation
typedef libra_error_t (*PFN_libra_preset_ctx_set_allow_rotation)(
    libra_preset_ctx_t *context, bool value);

/// Function pointer definition for
/// libra_preset_ctx_set_view_aspect_orientation
typedef libra_error_t (*PFN_libra_preset_ctx_set_view_aspect_orientation)(
    libra_preset_ctx_t *context, LIBRA_PRESET_CTX_ORIENTATION value);

/// Function pointer definition for
/// libra_preset_ctx_set_core_aspect_orientation
typedef libra_error_t (*PFN_libra_preset_ctx_set_core_aspect_orientation)(
    libra_preset_ctx_t *context, LIBRA_PRESET_CTX_ORIENTATION value);

/// Function pointer definition for
/// libra_preset_ctx_set_runtime
typedef libra_error_t (*PFN_libra_preset_ctx_set_runtime)(
    libra_preset_ctx_t *context, LIBRA_PRESET_CTX_RUNTIME value);

/// Function pointer definition for libra_error_errno
typedef LIBRA_ERRNO (*PFN_libra_error_errno)(libra_error_t error);

/// Function pointer definition for libra_error_print
typedef int32_t (*PFN_libra_error_print)(libra_error_t error);

/// Function pointer definition for libra_error_free
typedef int32_t (*PFN_libra_error_free)(libra_error_t *error);

/// Function pointer definition for libra_error_write
typedef int32_t (*PFN_libra_error_write)(libra_error_t error, char **out);

/// Function pointer definition for libra_error_free_string
typedef int32_t (*PFN_libra_error_free_string)(char **out);

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_init_context
typedef libra_error_t (*PFN_libra_gl_init_context)(libra_gl_loader_t loader);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_filter_chain_create
typedef libra_error_t (*PFN_libra_gl_filter_chain_create)(
    libra_shader_preset_t *preset, const struct filter_chain_gl_opt_t *options,
    libra_gl_filter_chain_t *out);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_filter_chain_frame
typedef libra_error_t (*PFN_libra_gl_filter_chain_frame)(
    libra_gl_filter_chain_t *chain, size_t frame_count,
    struct libra_source_image_gl_t image, struct libra_viewport_t viewport,
    struct libra_output_framebuffer_gl_t out, const float *mvp,
    const struct frame_gl_opt_t *opt);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_filter_chain_set_param
typedef libra_error_t (*PFN_libra_gl_filter_chain_set_param)(
    libra_gl_filter_chain_t *chain, const char *param_name, float value);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_filter_chain_get_param
typedef libra_error_t (*PFN_libra_gl_filter_chain_get_param)(
    libra_gl_filter_chain_t *chain, const char *param_name, float *out);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_filter_chain_set_active_pass_count
typedef libra_error_t (*PFN_libra_gl_filter_chain_set_active_pass_count)(
    libra_gl_filter_chain_t *chain, uint32_t value);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_filter_chain_get_active_pass_count
typedef libra_error_t (*PFN_libra_gl_filter_chain_get_active_pass_count)(
    libra_gl_filter_chain_t *chain, uint32_t *out);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Function pointer definition for
/// libra_gl_filter_chain_free
typedef libra_error_t (*PFN_libra_gl_filter_chain_free)(
    libra_gl_filter_chain_t *chain);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_create
typedef libra_error_t (*PFN_libra_vk_filter_chain_create)(
    libra_shader_preset_t *preset, struct libra_device_vk_t vulkan,
    const struct filter_chain_vk_opt_t *options, libra_vk_filter_chain_t *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_create_deferred
typedef libra_error_t (*PFN_libra_vk_filter_chain_create_deferred)(
    libra_shader_preset_t *preset, struct libra_device_vk_t vulkan,
    VkCommandBuffer command_buffer, const struct filter_chain_vk_opt_t *options,
    libra_vk_filter_chain_t *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_frame
typedef libra_error_t (*PFN_libra_vk_filter_chain_frame)(
    libra_vk_filter_chain_t *chain, VkCommandBuffer command_buffer,
    size_t frame_count, struct libra_source_image_vk_t image,
    struct libra_viewport_t viewport, struct libra_output_image_vk_t out,
    const float *mvp, const struct frame_vk_opt_t *opt);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_set_param
typedef libra_error_t (*PFN_libra_vk_filter_chain_set_param)(
    libra_vk_filter_chain_t *chain, const char *param_name, float value);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_get_param
typedef libra_error_t (*PFN_libra_vk_filter_chain_get_param)(
    libra_vk_filter_chain_t *chain, const char *param_name, float *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_set_active_pass_count
typedef libra_error_t (*PFN_libra_vk_filter_chain_set_active_pass_count)(
    libra_vk_filter_chain_t *chain, uint32_t value);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_get_active_pass_count
typedef libra_error_t (*PFN_libra_vk_filter_chain_get_active_pass_count)(
    libra_vk_filter_chain_t *chain, uint32_t *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Function pointer definition for
/// libra_vk_filter_chain_free
typedef libra_error_t (*PFN_libra_vk_filter_chain_free)(
    libra_vk_filter_chain_t *chain);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_create
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_create)(
    libra_shader_preset_t *preset, ID3D11Device *device,
    const struct filter_chain_d3d11_opt_t *options,
    libra_d3d11_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_create_deferred
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_create_deferred)(
    libra_shader_preset_t *preset, ID3D11Device *device,
    ID3D11DeviceContext *device_context,
    const struct filter_chain_d3d11_opt_t *options,
    libra_d3d11_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_frame
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_frame)(
    libra_d3d11_filter_chain_t *chain, ID3D11DeviceContext *device_context,
    size_t frame_count, struct libra_source_image_d3d11_t image,
    struct libra_viewport_t viewport, ID3D11RenderTargetView *out,
    const float *mvp, const struct frame_d3d11_opt_t *options);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_set_param
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_set_param)(
    libra_d3d11_filter_chain_t *chain, const char *param_name, float value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_get_param
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_get_param)(
    libra_d3d11_filter_chain_t *chain, const char *param_name, float *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_set_active_pass_count
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_set_active_pass_count)(
    libra_d3d11_filter_chain_t *chain, uint32_t value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_get_active_pass_count
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_get_active_pass_count)(
    libra_d3d11_filter_chain_t *chain, uint32_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Function pointer definition for
/// libra_d3d11_filter_chain_free
typedef libra_error_t (*PFN_libra_d3d11_filter_chain_free)(
    libra_d3d11_filter_chain_t *chain);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_create
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_create)(
    libra_shader_preset_t *preset, ID3D12Device *device,
    const struct filter_chain_d3d12_opt_t *options,
    libra_d3d12_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_create_deferred
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_create_deferred)(
    libra_shader_preset_t *preset, ID3D12Device *device,
    ID3D12GraphicsCommandList *command_list,
    const struct filter_chain_d3d12_opt_t *options,
    libra_d3d12_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_frame
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_frame)(
    libra_d3d12_filter_chain_t *chain, ID3D12GraphicsCommandList *command_list,
    size_t frame_count, struct libra_source_image_d3d12_t image,
    struct libra_viewport_t viewport, struct libra_output_image_d3d12_t out,
    const float *mvp, const struct frame_d3d12_opt_t *options);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_set_param
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_set_param)(
    libra_d3d12_filter_chain_t *chain, const char *param_name, float value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_get_param
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_get_param)(
    libra_d3d12_filter_chain_t *chain, const char *param_name, float *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_set_active_pass_count
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_set_active_pass_count)(
    libra_d3d12_filter_chain_t *chain, uint32_t value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_get_active_pass_count
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_get_active_pass_count)(
    libra_d3d12_filter_chain_t *chain, uint32_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Function pointer definition for
/// libra_d3d12_filter_chain_free
typedef libra_error_t (*PFN_libra_d3d12_filter_chain_free)(
    libra_d3d12_filter_chain_t *chain);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_create
typedef libra_error_t (*PFN_libra_mtl_filter_chain_create)(
    libra_shader_preset_t *preset, id<MTLCommandQueue> queue,
    const struct filter_chain_mtl_opt_t *options,
    libra_mtl_filter_chain_t *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_create_deferred
typedef libra_error_t (*PFN_libra_mtl_filter_chain_create_deferred)(
    libra_shader_preset_t *preset, id<MTLCommandQueue> queue,
    id<MTLCommandBuffer> command_buffer,
    const struct filter_chain_mtl_opt_t *options,
    libra_mtl_filter_chain_t *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_frame
typedef libra_error_t (*PFN_libra_mtl_filter_chain_frame)(
    libra_mtl_filter_chain_t *chain, id<MTLCommandBuffer> command_buffer,
    size_t frame_count, id<MTLTexture> image, struct libra_viewport_t viewport,
    id<MTLTexture> output, const float *mvp, const struct frame_mtl_opt_t *opt);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_set_param
typedef libra_error_t (*PFN_libra_mtl_filter_chain_set_param)(
    libra_mtl_filter_chain_t *chain, const char *param_name, float value);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_get_param
typedef libra_error_t (*PFN_libra_mtl_filter_chain_get_param)(
    libra_mtl_filter_chain_t *chain, const char *param_name, float *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_set_active_pass_count
typedef libra_error_t (*PFN_libra_mtl_filter_chain_set_active_pass_count)(
    libra_mtl_filter_chain_t *chain, uint32_t value);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_get_active_pass_count
typedef libra_error_t (*PFN_libra_mtl_filter_chain_get_active_pass_count)(
    libra_mtl_filter_chain_t *chain, uint32_t *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Function pointer definition for
/// libra_mtl_filter_chain_free
typedef libra_error_t (*PFN_libra_mtl_filter_chain_free)(
    libra_mtl_filter_chain_t *chain);
#endif

/// The current version of the librashader API.
/// Pass this into `version` for config structs.
///
/// API versions are backwards compatible. It is valid to load
/// a librashader C API instance for all API versions less than
/// or equal to LIBRASHADER_CURRENT_VERSION, and subsequent API
/// versions must remain backwards compatible.
/// ## API Versions
/// - API version 0: 0.1.0
/// - API version 1: 0.2.0
///     - Added rotation, total_subframes, current_subframes to frame options
///     - Added preset context API
///     - Added Metal runtime API
#define LIBRASHADER_CURRENT_VERSION 1

/// The current version of the librashader ABI.
/// Used by the loader to check ABI compatibility.
///
/// ABI version 0 is reserved as a sentinel value.
///
/// ABI versions are not backwards compatible. It is not
/// valid to load a librashader C API instance for any ABI
/// version not equal to LIBRASHADER_CURRENT_ABI.
/// ## ABI Versions
/// - ABI version 0: null instance (unloaded)
/// - ABI version 1: 0.1.0
#define LIBRASHADER_CURRENT_ABI 1

#ifdef __cplusplus
extern "C" {
#endif    // __cplusplus

/// Get the error code corresponding to this error object.
///
/// ## Safety
///   - `error` must be valid and initialized.
LIBRA_ERRNO libra_error_errno(libra_error_t error);

/// Print the error message.
///
/// If `error` is null, this function does nothing and returns 1. Otherwise,
/// this function returns 0.
/// ## Safety
///   - `error` must be a valid and initialized instance of `libra_error_t`.
int32_t libra_error_print(libra_error_t error);

/// Frees any internal state kept by the error.
///
/// If `error` is null, this function does nothing and returns 1. Otherwise,
/// this function returns 0. The resulting error object becomes null.
/// ## Safety
///   - `error` must be null or a pointer to a valid and initialized instance of
///   `libra_error_t`.
int32_t libra_error_free(libra_error_t *error);

/// Writes the error message into `out`
///
/// If `error` is null, this function does nothing and returns 1. Otherwise,
/// this function returns 0.
/// ## Safety
///   - `error` must be a valid and initialized instance of `libra_error_t`.
///   - `out` must be a non-null pointer. The resulting string must not be
///   modified.
int32_t libra_error_write(libra_error_t error, char **out);

/// Frees an error string previously allocated by `libra_error_write`.
///
/// After freeing, the pointer will be set to null.
/// ## Safety
///   - If `libra_error_write` is not null, it must point to a string previously
///   returned by `libra_error_write`.
///     Attempting to free anything else, including strings or objects from
///     other librashader functions, is immediate Undefined Behaviour.
int32_t libra_error_free_string(char **out);

/// Load a preset.
///
/// ## Safety
///  - `filename` must be either null or a valid, aligned pointer to a string
///  path to the shader preset.
///  - `out` must be either null, or an aligned pointer to an uninitialized or
///  invalid `libra_shader_preset_t`.
/// ## Returns
///  - If any parameters are null, `out` is unchanged, and this function returns
///  `LIBRA_ERR_INVALID_PARAMETER`.
libra_error_t libra_preset_create(const char *filename,
                                  libra_shader_preset_t *out);

/// Load a preset with the given wildcard context.
///
/// The wildcard context is immediately invalidated and must be recreated after
/// the preset is created.
///
/// Path information variables `PRESET_DIR` and `PRESET` will automatically be
/// filled in.
/// ## Safety
///  - `filename` must be either null or a valid, aligned pointer to a string
///  path to the shader preset.
///  - `context` must be either null or a valid, aligned pointer to a
///  initialized `libra_preset_ctx_t`.
///  - `context` is  invalidated after this function returns.
///  - `out` must be either null, or an aligned pointer to an uninitialized or
///  invalid `libra_shader_preset_t`.
/// ## Returns
///  - If any parameters are null, `out` is unchanged, and this function returns
///  `LIBRA_ERR_INVALID_PARAMETER`.
libra_error_t libra_preset_create_with_context(const char *filename,
                                               libra_preset_ctx_t *context,
                                               libra_shader_preset_t *out);

/// Free the preset.
///
/// If `preset` is null, this function does nothing. The resulting value in
/// `preset` then becomes null.
///
/// ## Safety
/// - `preset` must be a valid and aligned pointer to a `libra_shader_preset_t`.
libra_error_t libra_preset_free(libra_shader_preset_t *preset);

/// Set the value of the parameter in the preset.
///
/// ## Safety
/// - `preset` must be null or a valid and aligned pointer to a
/// `libra_shader_preset_t`.
/// - `name` must be null or a valid and aligned pointer to a string.
libra_error_t libra_preset_set_param(libra_shader_preset_t *preset,
                                     const char *name, float value);

/// Get the value of the parameter as set in the preset.
///
/// ## Safety
/// - `preset` must be null or a valid and aligned pointer to a shader preset.
/// - `name` must be null or a valid and aligned pointer to a string.
/// - `value` may be a pointer to a uninitialized `float`.
libra_error_t libra_preset_get_param(libra_shader_preset_t *preset,
                                     const char *name, float *value);

/// Pretty print the shader preset.
///
/// ## Safety
/// - `preset` must be null or a valid and aligned pointer to a
/// `libra_shader_preset_t`.
libra_error_t libra_preset_print(libra_shader_preset_t *preset);

/// Get a list of runtime parameters.
///
/// ## Safety
/// - `preset` must be null or a valid and aligned pointer to a
/// `libra_shader_preset_t`.
/// - `out` must be an aligned pointer to a `libra_preset_parameter_list_t`.
/// - The output struct should be treated as immutable. Mutating any struct
/// fields
///   in the returned struct may at best cause memory leaks, and at worse
///   cause undefined behaviour when later freed.
/// - It is safe to call `libra_preset_get_runtime_params` multiple times,
/// however
///   the output struct must only be freed once per call.
libra_error_t libra_preset_get_runtime_params(
    libra_shader_preset_t *preset, struct libra_preset_param_list_t *out);

/// Free the runtime parameters.
///
/// Unlike the other `free` functions provided by librashader,
/// `libra_preset_free_runtime_params` takes the struct directly.
/// The caller must take care to maintain the lifetime of any pointers
/// contained within the input `libra_preset_param_list_t`.
///
/// ## Safety
/// - Any pointers rooted at `parameters` becomes invalid after this function
/// returns,
///   including any strings accessible via the input
///   `libra_preset_param_list_t`. The caller must ensure that there are no live
///   pointers, aliased or unaliased, to data accessible via the input
///   `libra_preset_param_list_t`.
///
/// - Accessing any data pointed to via the input `libra_preset_param_list_t`
/// after it
///   has been freed is a use-after-free and is immediate undefined behaviour.
///
/// - If any struct fields of the input `libra_preset_param_list_t` was modified
/// from
///   their values given after `libra_preset_get_runtime_params`, this may
///   result in undefined behaviour.
libra_error_t libra_preset_free_runtime_params(
    struct libra_preset_param_list_t preset);

#if defined(LIBRA_RUNTIME_OPENGL)
/// Initialize the OpenGL Context for librashader.
///
/// This only has to be done once throughout the lifetime of the application,
/// unless for whatever reason you switch OpenGL loaders mid-flight.
///
/// ## Safety
/// Attempting to create a filter chain will fail if the GL context is not
/// initialized.
///
/// Reinitializing the OpenGL context with a different loader immediately
/// invalidates previous filter chain objects, and drawing with them causes
/// immediate undefined behaviour.
libra_error_t libra_gl_init_context(libra_gl_loader_t loader);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Create the filter chain given the shader preset.
///
/// The shader preset is immediately invalidated and must be recreated after
/// the filter chain is created.
///
/// ## Safety:
/// - `preset` must be either null, or valid and aligned.
/// - `options` must be either null, or valid and aligned.
/// - `out` must be aligned, but may be null, invalid, or uninitialized.
libra_error_t libra_gl_filter_chain_create(
    libra_shader_preset_t *preset, const struct filter_chain_gl_opt_t *options,
    libra_gl_filter_chain_t *out);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Draw a frame with the given parameters for the given filter chain.
///
/// ## Safety
/// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or
/// invalid, this
///    function will return an error.
/// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16
/// consecutive `float`
///    values for the model view projection matrix.
/// - `opt` may be null, or if it is not null, must be an aligned pointer to a
/// valid `frame_gl_opt_t`
///    struct.
/// - You must ensure that only one thread has access to `chain` before you call
/// this function. Only one
///   thread at a time may call this function. The thread
///   `libra_gl_filter_chain_frame` is called from must have its thread-local
///   OpenGL context initialized with the same context used to create the filter
///   chain.
libra_error_t libra_gl_filter_chain_frame(
    libra_gl_filter_chain_t *chain, size_t frame_count,
    struct libra_source_image_gl_t image, struct libra_viewport_t viewport,
    struct libra_output_framebuffer_gl_t out, const float *mvp,
    const struct frame_gl_opt_t *opt);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Sets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_gl_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_gl_filter_chain_set_param(libra_gl_filter_chain_t *chain,
                                              const char *param_name,
                                              float value);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Gets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_gl_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_gl_filter_chain_get_param(libra_gl_filter_chain_t *chain,
                                              const char *param_name,
                                              float *out);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Sets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_gl_filter_chain_t`.
libra_error_t libra_gl_filter_chain_set_active_pass_count(
    libra_gl_filter_chain_t *chain, uint32_t value);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Gets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_gl_filter_chain_t`.
libra_error_t libra_gl_filter_chain_get_active_pass_count(
    libra_gl_filter_chain_t *chain, uint32_t *out);
#endif

#if defined(LIBRA_RUNTIME_OPENGL)
/// Free a GL filter chain.
///
/// The resulting value in `chain` then becomes null.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_gl_filter_chain_t`.
libra_error_t libra_gl_filter_chain_free(libra_gl_filter_chain_t *chain);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Create the filter chain given the shader preset.
///
/// The shader preset is immediately invalidated and must be recreated after
/// the filter chain is created.
///
/// ## Safety:
/// - The handles provided in `vulkan` must be valid for the command buffers
/// that
///   `libra_vk_filter_chain_frame` will write to.
///    created with the `VK_KHR_dynamic_rendering` extension.
/// - `preset` must be either null, or valid and aligned.
/// - `options` must be either null, or valid and aligned.
/// - `out` must be aligned, but may be null, invalid, or uninitialized.
libra_error_t libra_vk_filter_chain_create(
    libra_shader_preset_t *preset, struct libra_device_vk_t vulkan,
    const struct filter_chain_vk_opt_t *options, libra_vk_filter_chain_t *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Create the filter chain given the shader preset deferring and GPU-side
/// initialization to the caller. This function therefore requires no external
/// synchronization of the device queue.
///
/// The shader preset is immediately invalidated and must be recreated after
/// the filter chain is created.
///
/// ## Safety:
/// - The handles provided in `vulkan` must be valid for the command buffers
/// that
///   `libra_vk_filter_chain_frame` will write to.
/// - `preset` must be either null, or valid and aligned.
/// - `options` must be either null, or valid and aligned.
/// - `out` must be aligned, but may be null, invalid, or uninitialized.
///
/// The provided command buffer must be ready for recording and contain no prior
/// commands. The caller is responsible for ending the command buffer and
/// immediately submitting it to a graphics queue. The command buffer must be
/// completely executed before calling `libra_vk_filter_chain_frame`.
libra_error_t libra_vk_filter_chain_create_deferred(
    libra_shader_preset_t *preset, struct libra_device_vk_t vulkan,
    VkCommandBuffer command_buffer, const struct filter_chain_vk_opt_t *options,
    libra_vk_filter_chain_t *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Records rendering commands for a frame with the given parameters for the
/// given filter chain to the input command buffer.
///
/// * The input image must be in the `VK_SHADER_READ_ONLY_OPTIMAL` layout.
/// * The output image must be in `VK_COLOR_ATTACHMENT_OPTIMAL` layout.
///
/// librashader **will not** create a pipeline barrier for the final pass. The
/// output image will remain in `VK_COLOR_ATTACHMENT_OPTIMAL` after all shader
/// passes. The caller must transition the output image to the final layout.
///
/// ## Safety
/// - `libra_vk_filter_chain_frame` **must not be called within a RenderPass**.
/// - `command_buffer` must be a valid handle to a `VkCommandBuffer` that is
/// ready for recording.
/// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or
/// invalid, this
///    function will return an error.
/// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16
/// consecutive `float`
///    values for the model view projection matrix.
/// - `opt` may be null, or if it is not null, must be an aligned pointer to a
/// valid `frame_vk_opt_t`
///    struct.
/// - You must ensure that only one thread has access to `chain` before you call
/// this function. Only one
///   thread at a time may call this function.
libra_error_t libra_vk_filter_chain_frame(
    libra_vk_filter_chain_t *chain, VkCommandBuffer command_buffer,
    size_t frame_count, struct libra_source_image_vk_t image,
    struct libra_viewport_t viewport, struct libra_output_image_vk_t out,
    const float *mvp, const struct frame_vk_opt_t *opt);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Sets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_vk_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_vk_filter_chain_set_param(libra_vk_filter_chain_t *chain,
                                              const char *param_name,
                                              float value);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Gets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_vk_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_vk_filter_chain_get_param(libra_vk_filter_chain_t *chain,
                                              const char *param_name,
                                              float *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Sets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_vk_filter_chain_t`.
libra_error_t libra_vk_filter_chain_set_active_pass_count(
    libra_vk_filter_chain_t *chain, uint32_t value);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Gets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_vk_filter_chain_t`.
libra_error_t libra_vk_filter_chain_get_active_pass_count(
    libra_vk_filter_chain_t *chain, uint32_t *out);
#endif

#if defined(LIBRA_RUNTIME_VULKAN)
/// Free a Vulkan filter chain.
///
/// The resulting value in `chain` then becomes null.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_vk_filter_chain_t`.
libra_error_t libra_vk_filter_chain_free(libra_vk_filter_chain_t *chain);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
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
libra_error_t libra_d3d11_filter_chain_create(
    libra_shader_preset_t *preset, ID3D11Device *device,
    const struct filter_chain_d3d11_opt_t *options,
    libra_d3d11_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Create the filter chain given the shader preset, deferring and GPU-side
/// initialization to the caller. This function is therefore requires no
/// external synchronization of the immediate context, as long as the immediate
/// context is not used as the input context, nor of the device, as long as the
/// device is not single-threaded only.
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
/// The provided context must either be immediate, or immediately submitted
/// after this function returns, **before drawing frames**, or lookup textures
/// will fail to load and the filter chain will be in an invalid state.
///
/// If the context is deferred, it must be ready for command recording, and have
/// no prior commands recorded. No commands shall be recorded after, the caller
/// must immediately call
/// [`FinishCommandList`](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-finishcommandlist)
/// and execute the command list on the immediate context after this function
/// returns.
///
/// If the context is immediate, then access to the immediate context requires
/// external synchronization.
libra_error_t libra_d3d11_filter_chain_create_deferred(
    libra_shader_preset_t *preset, ID3D11Device *device,
    ID3D11DeviceContext *device_context,
    const struct filter_chain_d3d11_opt_t *options,
    libra_d3d11_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Draw a frame with the given parameters for the given filter chain.
///
/// If `device_context` is null, then commands are recorded onto the immediate
/// context. Otherwise, it will record commands onto the provided context. If
/// the context is deferred, librashader will not finalize command lists. The
/// context must otherwise be associated with the `ID3D11Device`
///
/// ## Safety
/// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or
/// invalid, this
///    function will return an error.
/// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16
/// consecutive `float`
///    values for the model view projection matrix.
/// - `opt` may be null, or if it is not null, must be an aligned pointer to a
/// valid `frame_d3d11_opt_t`
///    struct.
/// - `out` must not be null.
/// - `image.handle` must not be null.
/// - If `device_context` is null, commands will be recorded onto the immediate
/// context of the `ID3D11Device`
///   this filter chain was created with. The context must otherwise be
///   associated with the `ID3D11Device` the filter chain was created with.
/// - You must ensure that only one thread has access to `chain` before you call
/// this function. Only one
///   thread at a time may call this function.
libra_error_t libra_d3d11_filter_chain_frame(
    libra_d3d11_filter_chain_t *chain, ID3D11DeviceContext *device_context,
    size_t frame_count, struct libra_source_image_d3d11_t image,
    struct libra_viewport_t viewport, ID3D11RenderTargetView *out,
    const float *mvp, const struct frame_d3d11_opt_t *options);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Sets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d11_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_d3d11_filter_chain_set_param(
    libra_d3d11_filter_chain_t *chain, const char *param_name, float value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Gets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d11_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_d3d11_filter_chain_get_param(
    libra_d3d11_filter_chain_t *chain, const char *param_name, float *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Sets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d11_filter_chain_t`.
libra_error_t libra_d3d11_filter_chain_set_active_pass_count(
    libra_d3d11_filter_chain_t *chain, uint32_t value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Gets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d11_filter_chain_t`.
libra_error_t libra_d3d11_filter_chain_get_active_pass_count(
    libra_d3d11_filter_chain_t *chain, uint32_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D11))
/// Free a D3D11 filter chain.
///
/// The resulting value in `chain` then becomes null.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d11_filter_chain_t`.
libra_error_t libra_d3d11_filter_chain_free(libra_d3d11_filter_chain_t *chain);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
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
libra_error_t libra_d3d12_filter_chain_create(
    libra_shader_preset_t *preset, ID3D12Device *device,
    const struct filter_chain_d3d12_opt_t *options,
    libra_d3d12_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Create the filter chain given the shader preset deferring and GPU-side
/// initialization to the caller. This function therefore requires no external
/// synchronization of the device queue.
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
/// The provided command list must be ready for recording and contain no prior
/// commands. The caller is responsible for ending the command list and
/// immediately submitting it to a graphics queue. The command list must be
/// completely executed before calling `libra_d3d12_filter_chain_frame`.
libra_error_t libra_d3d12_filter_chain_create_deferred(
    libra_shader_preset_t *preset, ID3D12Device *device,
    ID3D12GraphicsCommandList *command_list,
    const struct filter_chain_d3d12_opt_t *options,
    libra_d3d12_filter_chain_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Records rendering commands for a frame with the given parameters for the
/// given filter chain to the input command list.
///
/// * The input image must be in the
/// `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` resource state.
/// * The output image must be in `D3D12_RESOURCE_STATE_RENDER_TARGET` resource
/// state.
///
/// librashader **will not** create a resource barrier for the final pass. The
/// output image will remain in `D3D12_RESOURCE_STATE_RENDER_TARGET` after all
/// shader passes. The caller must transition the output image to the final
/// resource state.
///
/// ## Safety
/// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or
/// invalid, this
///    function will return an error.
/// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16
/// consecutive `float`
///    values for the model view projection matrix.
/// - `opt` may be null, or if it is not null, must be an aligned pointer to a
/// valid `frame_d3d12_opt_t`
///    struct.
/// - `out` must be a descriptor handle to a render target view.
/// - `image.resource` must not be null.
/// - `command_list` must be a non-null pointer to a `ID3D12GraphicsCommandList`
/// that is open,
///    and must be associated with the `ID3D12Device` this filter chain was
///    created with.
/// - You must ensure that only one thread has access to `chain` before you call
/// this function. Only one
///   thread at a time may call this function.
libra_error_t libra_d3d12_filter_chain_frame(
    libra_d3d12_filter_chain_t *chain, ID3D12GraphicsCommandList *command_list,
    size_t frame_count, struct libra_source_image_d3d12_t image,
    struct libra_viewport_t viewport, struct libra_output_image_d3d12_t out,
    const float *mvp, const struct frame_d3d12_opt_t *options);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Sets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d12_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_d3d12_filter_chain_set_param(
    libra_d3d12_filter_chain_t *chain, const char *param_name, float value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Gets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d12_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_d3d12_filter_chain_get_param(
    libra_d3d12_filter_chain_t *chain, const char *param_name, float *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Sets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d12_filter_chain_t`.
libra_error_t libra_d3d12_filter_chain_set_active_pass_count(
    libra_d3d12_filter_chain_t *chain, uint32_t value);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Gets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d12_filter_chain_t`.
libra_error_t libra_d3d12_filter_chain_get_active_pass_count(
    libra_d3d12_filter_chain_t *chain, uint32_t *out);
#endif

#if (defined(_WIN32) && defined(LIBRA_RUNTIME_D3D12))
/// Free a D3D12 filter chain.
///
/// The resulting value in `chain` then becomes null.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_d3d12_filter_chain_t`.
libra_error_t libra_d3d12_filter_chain_free(libra_d3d12_filter_chain_t *chain);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
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
libra_error_t libra_mtl_filter_chain_create(
    libra_shader_preset_t *preset, id<MTLCommandQueue> queue,
    const struct filter_chain_mtl_opt_t *options,
    libra_mtl_filter_chain_t *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Create the filter chain given the shader preset deferring and GPU-side
/// initialization to the caller. This function therefore requires no external
/// synchronization of the device queue.
///
/// The shader preset is immediately invalidated and must be recreated after
/// the filter chain is created.
///
/// ## Safety:
/// - `queue` must be valid for the command buffers
///     that `libra_mtl_filter_chain_frame` will write to.
/// - `queue` must be a reference to a `id<MTLCommandQueue>`.
/// - `command_buffer` must be a valid reference to a `MTLCommandBuffer` that is
/// not already encoding.
/// - `preset` must be either null, or valid and aligned.
/// - `options` must be either null, or valid and aligned.
/// - `out` must be aligned, but may be null, invalid, or uninitialized.
///
/// The provided command buffer must be ready for recording and contain no prior
/// commands. The caller is responsible for ending the command buffer and
/// immediately submitting it to a graphics queue. The command buffer must be
/// completely executed before calling `libra_mtl_filter_chain_frame`.
libra_error_t libra_mtl_filter_chain_create_deferred(
    libra_shader_preset_t *preset, id<MTLCommandQueue> queue,
    id<MTLCommandBuffer> command_buffer,
    const struct filter_chain_mtl_opt_t *options,
    libra_mtl_filter_chain_t *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Records rendering commands for a frame with the given parameters for the
/// given filter chain to the input command buffer.
///
/// ## Safety
/// - `command_buffer` must be a valid reference to a `MTLCommandBuffer` that is
/// not already encoding.
/// - `chain` may be null, invalid, but not uninitialized. If `chain` is null or
/// invalid, this
///    function will return an error.
/// - `mvp` may be null, or if it is not null, must be an aligned pointer to 16
/// consecutive `float`
///    values for the model view projection matrix.
/// - `opt` may be null, or if it is not null, must be an aligned pointer to a
/// valid `frame_mtl_opt_t`
///    struct.
/// - You must ensure that only one thread has access to `chain` before you call
/// this function. Only one
///   thread at a time may call this function.
libra_error_t libra_mtl_filter_chain_frame(
    libra_mtl_filter_chain_t *chain, id<MTLCommandBuffer> command_buffer,
    size_t frame_count, id<MTLTexture> image, struct libra_viewport_t viewport,
    id<MTLTexture> output, const float *mvp, const struct frame_mtl_opt_t *opt);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Sets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_mtl_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_mtl_filter_chain_set_param(libra_mtl_filter_chain_t *chain,
                                               const char *param_name,
                                               float value);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Gets a parameter for the filter chain.
///
/// If the parameter does not exist, returns an error.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_mtl_filter_chain_t`.
/// - `param_name` must be either null or a null terminated string.
libra_error_t libra_mtl_filter_chain_get_param(libra_mtl_filter_chain_t *chain,
                                               const char *param_name,
                                               float *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Sets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_mtl_filter_chain_t`.
libra_error_t libra_mtl_filter_chain_set_active_pass_count(
    libra_mtl_filter_chain_t *chain, uint32_t value);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Gets the number of active passes for this chain.
///
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_mtl_filter_chain_t`.
libra_error_t libra_mtl_filter_chain_get_active_pass_count(
    libra_mtl_filter_chain_t *chain, uint32_t *out);
#endif

#if (defined(__APPLE__) && defined(LIBRA_RUNTIME_METAL) && defined(__OBJC__))
/// Free a Metal filter chain.
///
/// The resulting value in `chain` then becomes null.
/// ## Safety
/// - `chain` must be either null or a valid and aligned pointer to an
/// initialized `libra_mtl_filter_chain_t`.
libra_error_t libra_mtl_filter_chain_free(libra_mtl_filter_chain_t *chain);
#endif

/// Get the ABI version of the loaded instance.
LIBRASHADER_ABI_VERSION libra_instance_abi_version(void);

/// Get the API version of the loaded instance.
LIBRASHADER_API_VERSION libra_instance_api_version(void);

/// Create a wildcard context
///
/// The C API does not allow directly setting certain variables
///
/// - `PRESET_DIR` and `PRESET` are inferred on preset creation.
/// - `VID-DRV-SHADER-EXT` and `VID-DRV-PRESET-EXT` are always set to `slang`
/// and `slangp` for librashader.
/// - `VID-FINAL-ROT` is automatically calculated as the sum of `VID-USER-ROT`
/// and `CORE-REQ-ROT` if either are present.
///
/// These automatically inferred variables, as well as all other variables can
/// be overridden with `libra_preset_ctx_set_param`, but the expected string
/// values must be provided. See
/// https://github.com/libretro/RetroArch/pull/15023 for a list of expected
/// string values.
///
/// No variables can be removed once added to the context, however subsequent
/// calls to set the same variable will overwrite the expected variable.
/// ## Safety
///  - `out` must be either null, or an aligned pointer to an uninitialized or
///  invalid `libra_preset_ctx_t`.
/// ## Returns
///  - If any parameters are null, `out` is unchanged, and this function returns
///  `LIBRA_ERR_INVALID_PARAMETER`.
libra_error_t libra_preset_ctx_create(libra_preset_ctx_t *out);

/// Free the wildcard context.
///
/// If `context` is null, this function does nothing. The resulting value in
/// `context` then becomes null.
///
/// ## Safety
/// - `context` must be a valid and aligned pointer to a `libra_preset_ctx_t`
libra_error_t libra_preset_ctx_free(libra_preset_ctx_t *context);

/// Set the core name (`CORE`) variable in the context
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
/// - `name` must be null or a valid and aligned pointer to a string.
libra_error_t libra_preset_ctx_set_core_name(libra_preset_ctx_t *context,
                                             const char *name);

/// Set the content directory (`CONTENT-DIR`) variable in the context.
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
/// - `name` must be null or a valid and aligned pointer to a string.
libra_error_t libra_preset_ctx_set_content_dir(libra_preset_ctx_t *context,
                                               const char *name);

/// Set a custom string variable in context.
///
/// If the path contains this variable when loading a preset, it will be
/// replaced with the provided contents.
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
/// - `name` must be null or a valid and aligned pointer to a string.
/// - `value` must be null or a valid and aligned pointer to a string.
libra_error_t libra_preset_ctx_set_param(libra_preset_ctx_t *context,
                                         const char *name, const char *value);

/// Set the graphics runtime (`VID-DRV`) variable in the context.
///
/// Note that librashader only supports the following runtimes.
///
/// - Vulkan
/// - GLCore
/// - Direct3D11
/// - Direct3D12
/// - Metal
///
/// This will also set the appropriate video driver extensions.
///
/// For librashader, `VID-DRV-SHADER-EXT` and `VID-DRV-PRESET-EXT` are always
/// `slang` and `slangp`. To override this, use `libra_preset_ctx_set_param`.
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
/// - `name` must be null or a valid and aligned pointer to a string.
libra_error_t libra_preset_ctx_set_runtime(libra_preset_ctx_t *context,
                                           LIBRA_PRESET_CTX_RUNTIME value);

/// Set the core requested rotation (`CORE-REQ-ROT`) variable in the context.
///
/// Rotation is represented by quarter rotations around the unit circle.
/// For example. `0` = 0deg, `1` = 90deg, `2` = 180deg, `3` = 270deg, `4` =
/// 0deg.
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
libra_error_t libra_preset_ctx_set_core_rotation(libra_preset_ctx_t *context,
                                                 uint32_t value);

/// Set the user rotation (`VID-USER-ROT`) variable in the context.
///
/// Rotation is represented by quarter rotations around the unit circle.
/// For example. `0` = 0deg, `1` = 90deg, `2` = 180deg, `3` = 270deg, `4` =
/// 0deg.
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
libra_error_t libra_preset_ctx_set_user_rotation(libra_preset_ctx_t *context,
                                                 uint32_t value);

/// Set the screen orientation (`SCREEN-ORIENT`) variable in the context.
///
/// Orientation is represented by quarter rotations around the unit circle.
/// For example. `0` = 0deg, `1` = 90deg, `2` = 180deg, `3` = 270deg, `4` =
/// 0deg.
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
libra_error_t libra_preset_ctx_set_screen_orientation(
    libra_preset_ctx_t *context, uint32_t value);

/// Set whether or not to allow rotation (`VID-ALLOW-CORE-ROT`) variable in the
/// context.
///
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
libra_error_t libra_preset_ctx_set_allow_rotation(libra_preset_ctx_t *context,
                                                  bool value);

/// Set the view aspect orientation (`VIEW-ASPECT-ORIENT`) variable in the
/// context.
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
libra_error_t libra_preset_ctx_set_view_aspect_orientation(
    libra_preset_ctx_t *context, LIBRA_PRESET_CTX_ORIENTATION value);

/// Set the core aspect orientation (`CORE-ASPECT-ORIENT`) variable in the
/// context.
/// ## Safety
/// - `context` must be null or a valid and aligned pointer to a
/// `libra_preset_ctx_t`.
libra_error_t libra_preset_ctx_set_core_aspect_orientation(
    libra_preset_ctx_t *context, LIBRA_PRESET_CTX_ORIENTATION value);

#ifdef __cplusplus
}    // extern "C"
#endif    // __cplusplus

#endif /* __LIBRASHADER_H__ */
