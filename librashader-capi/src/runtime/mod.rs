//! librashader runtime C APIs.
#[doc(cfg(feature = "runtime-opengl"))]
#[cfg(feature = "runtime-opengl")]
pub mod gl;

#[doc(cfg(feature = "runtime-vulkan"))]
#[cfg(feature = "runtime-vulkan")]
pub mod vk;

#[doc(cfg(all(target_os = "windows", feature = "runtime-d3d11")))]
#[cfg(any(
    feature = "__cbindgen_internal",
    all(target_os = "windows", feature = "runtime-d3d11")
))]
pub mod d3d11;

#[doc(cfg(all(target_os = "windows", feature = "runtime-d3d12")))]
#[cfg(any(
    feature = "__cbindgen_internal",
    all(target_os = "windows", feature = "runtime-d3d12")
))]
pub mod d3d12;

#[doc(cfg(all(target_vendor = "apple", feature = "runtime-metal")))]
#[cfg(any(
    feature = "__cbindgen_internal",
    all(
        target_vendor = "apple",
        feature = "runtime-metal",
        feature = "__cbindgen_internal_objc"
    )
))]
pub mod mtl;
