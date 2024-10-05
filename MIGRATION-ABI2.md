# Migrating to librashader ABI 2

librashader version 0.5.0 introduces an ABI change to the C ABI that is incompatible with prior versions. 

If a version of librashader prior to 0.5.0 was linked via `libashader_ld.h`, it should _correctly_ fail to load when attempting
to load an instance of librashader 0.5.0 or later. 

This document is only relevant to consumers of the librashader C ABI. Rust users should consult [docs.rs/librashader](https://docs.rs/librashader/latest/librashader/).

## `LIBRASHADER_CURRENT_ABI` change

`LIBRASHADER_CURRENT_ABI` has changed from `1` to `2`. There is no change to `LIBRASHADER_CURRENT_API`, as new features
have not been added. Option structs should continue to pass `LIBRASHADER_CURRENT_API` for the latest available feature set.

## `SONAME` change

On Linux, the canonical `SONAME` of `librashader.so` has changed from `librashader.so.1` to `librashader.so.2`. This will
affect consumers that link via the linkage table with `-lrashader` or equivalent rather than through  `libashader_ld.h`.

## `libra_preset` changes

* The `_internal_alloc` field from `libra_preset_param_list_t` has been removed. This change was made to reduce the potential
  surface area for undefined behaviour to occur. If librashader was used correctly, there is no code change required.

## `LIBRA_RUNTIME_VULKAN` changes
The following changes are applicable if `LIBRA_RUNTIME_VULKAN` is defined.

* The `libra_output_image_vk_t` and `libra_source_image_vk_t` structs have been replaced with `libra_image_vk_t`. 
  * `libra_image_vk_t`has the same layout and semantics for `libra_source_image_vk_t`. 
  * When passed as `out`, you should now pass what was previously `.width` and `.height` of `libra_viewport_t` to the same fields in `libra_image_vk_t`. The `handle` and `format`
    fields retain the same semantics as `libra_output_image_vk_t`.
* A field `queue` of type `VkQueue` was added to `libra_device_vk_t`. This field can be `NULL`. If not `NULL`, it is the handle to the `VkQueue` graphics queue
  to use for the filter chain. If `NULL`, a suitable queue will be chosen.
* The `image` and `out` parameters of `libra_vk_filter_chain_frame` has changed from `libra_source_image_vk_t` and `libra_output_image_vk_t`, to `libra_image_vk_t`.
* In `libra_vk_filter_chain_frame`, the position of the `viewport` parameter has moved to after the `out` parameter, and its type has changed from `libra_viewport_t` to `libra_viewport_t *`, which is allowed to be `NULL`.
  See [`libra_viewport_t` changes](#libra_viewport_t-changes) for more details.
* The `chain` parameter of `libra_vk_filter_chain_get_param` has been made `const`.
* It is always thread safe to call `libra_vk_filter_chain_set_param` from any thread [^1].

## `LIBRA_RUNTIME_OPENGL` changes
The following changes are applicable if `LIBRA_RUNTIME_OPENGL` is defined.
* The `libra_gl_init_context` function has been removed.
* The function `libra_gl_filter_chain_create` now accepts a `loader` parameter of type `libra_gl_loader_t`. This will be the OpenGL loader used to create the filter chain, previously passed to `libra_gl_init_context`
  The filter chain will be created against the current OpenGL context. 
* The `libra_output_framebuffer_gl_t` and `libra_source_image_gl_t` structs have been replaced with `libra_image_gl_t`.
  * `libra_image_gl_t`has the same layout and semantics for `libra_source_image_gl_t`.
  * When passed as `out`, you should now pass what was previously `.width` and `.height` of `libra_viewport_t` to the same fields in `libra_image_gl_t`. The `handle` and `format`
    fields retain the same semantics as `libra_output_image_gl_t`.
  * The `fbo` field previously in `libra_output_image_gl_t` is no longer necessary. librashader will now internally manage a framebuffer object to write to the provided texture.
* In `libra_gl_filter_chain_frame`, the position of the `viewport` parameter has moved to after the `out` parameter, and its type has changed from `libra_viewport_t` to `libra_viewport_t *`, which is allowed to be `NULL`.
  See [`libra_viewport_t` changes](#libra_viewport_t-changes) for more details.
* The `chain` parameter of `libra_gl_filter_chain_get_param` has been made `const`.
* It is always thread safe to call `libra_gl_filter_chain_set_param` from any thread [^1].

## `LIBRA_RUNTIME_D3D11` changes
The following changes are applicable if `LIBRA_RUNTIME_D3D11` is defined.

* The `image` parameter of `libra_d3d11_filter_chain_frame` has changed from `libra_source_image_d3d11_t` to `ID3D11ShaderResourceView *`.
  * You should now pass what was previously the `.handle` field of `libra_source_image_d3d11_t` field directly as `image` to `libra_d3d11_filter_chain_frame`.
* The `libra_source_image_d3d11_t` struct has been removed.
* In `libra_d3d11_filter_chain_frame`, the position of the `viewport` parameter has moved to after the `out` parameter, and its type has changed from `libra_viewport_t` to `libra_viewport_t *`, which is allowed to be `NULL`.
  See [`libra_viewport_t` changes](#libra_viewport_t-changes) for more details.
* The `chain` parameter of `libra_d3d11_filter_chain_get_param` has been made `const`.
* It is always thread safe to call `libra_d3d11_filter_chain_set_param` from any thread [^1].

## `LIBRA_RUNTIME_D3D12` changes
The following changes are applicable if `LIBRA_RUNTIME_D3D12` is defined.

* The lifetime of resources will not be extended past the call to the `libra_d3d12_filter_chain_frame` function. In other words, the refcount for any resources passed into the function
  will no longer be changed, and it is explicitly the responsibility of the caller to ensure any resources remain alive until the `ID3D12GraphicsCommandList` provided is submitted.
* The fields `format`, `width`, and `height` have been removed from `libra_source_image_d3d12_t`. 
* The field `descriptor` now comes before the field `resource` in the layout of `libra_source_image_d3d12_t`.
* The fields `width` and `height` have been added to `libra_output_image_d3d12_t`. 
  * You should now pass what was previously `.width` and `.height` of `libra_viewport_t` to these new fields in `libra_output_image_d3d12_t`.
* The `image` parameter of `libra_d3d12_filter_chain_frame` has changed from `libra_source_image_d3d12_t` to `libra_image_d3d12_t`.
  * To maintain the previous behaviour, `.image_type` of the `libra_image_d3d12_t` should be set to `LIBRA_D3D12_IMAGE_TYPE_SOURCE_IMAGE`, and `.handle.source` should be the `libra_source_image_d3d12_t`struct.
* The `out` parameter of `libra_d3d12_filter_chain_frame` has changed from `libra_output_image_d3d11_t` to `libra_image_d3d12_t`.
  * To maintain the previous behaviour, `.image_type` of the `libra_image_d3d12_t` should be set to `LIBRA_D3D12_IMAGE_TYPE_OUTPUT_IMAGE`, and `.handle.output` should be the `libra_output_image_d3d12_t`struct.
* Any `libra_image_d3d12_t` can now optionally pass only the `ID3D12Resource *` for the texture by setting `.image_type` to `LIBRA_D3D12_IMAGE_TYPE_RESOURCE` and setting `.handle.resource` to the resource pointer.
  *  If using `LIBRA_D3D12_IMAGE_TYPE_RESOURCE`, shader resource view and render target view descriptors for the input and output images will be internally allocated by the filter chain. This may result in marginally worse performance.
* In `libra_d3d12_filter_chain_frame`, the position of the `viewport` parameter has moved to after the `out` parameter, and its type has changed from `libra_viewport_t` to `libra_viewport_t *`, which is allowed to be `NULL`.
  See [`libra_viewport_t` changes](#libra_viewport_t-changes) for more details.
* The `chain` parameter of `libra_d3d12_filter_chain_get_param` has been made `const`.
* It is always thread safe to call `libra_d3d12_filter_chain_set_param` from any thread [^1].

## `LIBRA_RUNTIME_D3D9` changes
The following changes are applicable if `LIBRA_RUNTIME_D3D9` is defined.

* In `libra_d3d9_filter_chain_frame`, the position of the `viewport` parameter has moved to after the `out` parameter, and its type has changed from `libra_viewport_t` to `libra_viewport_t *`, which is allowed to be `NULL`.
  See [`libra_viewport_t` changes](#libra_viewport_t-changes) for more details.
* The `chain` parameter of `libra_d3d9_filter_chain_get_param` has been made `const`.
* It is always thread safe to call `libra_d3d9_filter_chain_set_param` from any thread [^1].

## `LIBRA_RUNTIME_METAL` changes
The following changes are applicable if `LIBRA_RUNTIME_METAL` is defined.

* In `libra_mtl_filter_chain_frame`, the position of the `viewport` parameter has moved to after the `out` parameter, and its type has changed from `libra_viewport_t` to `libra_viewport_t *`, which is allowed to be `NULL`.
  See [`libra_viewport_t` changes](#libra_viewport_t-changes) for more details.
* The `chain` parameter of `libra_mtl_filter_chain_get_param` has been made `const`.
* It is always thread safe to call `libra_mtl_filter_chain_set_param` from any thread [^1].

## `libra_viewport_t` changes

All `viewport` parameters for `libra_*_filter_chain_frame` now take a *pointer* to a `libra_viewport_t` struct. In ABI 1, the semantics of `libra_viewport_t` 
was unspecified (but not undefined behaviour) if `width` and `height` did not match the width and height of the output texture. 

This caused confusion as to what the actual purpose of the `width` and `height` fields were. The behaviour differed across runtimes:
In some runtimes, it specified the size of the output texture, in others they were used to set the clipping rect for the render target.

In ABI 2, the semantics of `viewport` as a parameter in `libra_*_filter_chain_frame` are as specified.

* If `viewport` is `NULL`, then this will be the same as the **specified** behaviour in ABI 1&mdash;that is, as if the behaviour where `width` and `height` of `libra_viewport_t` were **equal** to that of the output texture.
  * In other words, if `viewport` is `NULL`, then it will set the render viewport to be equal to the width and height of the output texture.
* If `viewport` is not `NULL`, then the following occurs.
  * The width and height of the viewport rectangle for the output quad will be set to `viewport.width` and `viewport.height` respectively.
  * The origin point of the viewport rectangle will be set to (`x`, `y`), **in the native coordinate system of the runtime**.
    * This behaviour may change over an _API_ version bump. **API version 1 will always retain the behaviour specified here.**
  * The scissor rectangle will be set to the same size and origin as the viewport rectangle.

[^1]: This has been the case since librashader 0.4.0 on ABI 1. ABI 2 codifies this guarantee: any loosening in the thread-safety guarantees of `libra_*_filter_chain_set_param` in the future may only change across an _API_ version bump. **API version 1 will always retain the behaviour specified here.**
