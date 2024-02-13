//! C API for the librashader OpenGL Runtime (`libra_vk_*`).

mod filter_chain;
pub use filter_chain::*;
const _: () = crate::assert_thread_safe::<librashader::runtime::vk::FilterChain>();
