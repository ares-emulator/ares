//! librashader runtime C APIs.
#[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime-opengl")))]
#[cfg(feature = "runtime-opengl")]
pub mod gl;

#[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime-vulkan")))]
#[cfg(feature = "runtime-vulkan")]
pub mod vk;

#[cfg_attr(
    feature = "docsrs",
    doc(cfg(all(target_os = "windows", feature = "runtime-d3d11")))
)]
#[cfg(any(
    feature = "__cbindgen_internal",
    all(target_os = "windows", feature = "runtime-d3d11")
))]
pub mod d3d11;

#[cfg_attr(
    feature = "docsrs",
    doc(cfg(all(target_os = "windows", feature = "runtime-d3d9")))
)]
#[cfg(any(
    feature = "__cbindgen_internal",
    all(target_os = "windows", feature = "runtime-d3d9")
))]
pub mod d3d9;

#[cfg_attr(
    feature = "docsrs",
    doc(cfg(all(target_os = "windows", feature = "runtime-d3d12")))
)]
#[cfg(any(
    feature = "__cbindgen_internal",
    all(target_os = "windows", feature = "runtime-d3d12")
))]
pub mod d3d12;

#[cfg_attr(
    feature = "docsrs",
    doc(cfg(all(target_vendor = "apple", feature = "runtime-metal")))
)]
#[cfg(any(
    feature = "__cbindgen_internal",
    all(
        target_vendor = "apple",
        feature = "runtime-metal",
        feature = "__cbindgen_internal_objc"
    )
))]
pub mod mtl;
