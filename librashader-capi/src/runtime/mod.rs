//! librashader runtime C APIs.
#[doc(cfg(feature = "runtime-opengl"))]
#[cfg(feature = "runtime-opengl")]
pub mod gl;

#[doc(cfg(feature = "runtime-vulkan"))]
#[cfg(feature = "runtime-vulkan")]
pub mod vk;

#[doc(cfg(all(target_os = "windows", feature = "runtime-d3d11")))]
#[cfg(all(target_os = "windows", feature = "runtime-d3d11"))]
pub mod d3d11;

#[doc(cfg(all(target_os = "windows", feature = "runtime-d3d12")))]
#[cfg(all(target_os = "windows", feature = "runtime-d3d12"))]
pub mod d3d12;
