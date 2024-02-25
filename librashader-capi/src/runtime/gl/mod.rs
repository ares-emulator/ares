//! C API for the librashader OpenGL Runtime (`libra_gl_*`).

mod filter_chain;
pub use filter_chain::*;
const _: () = crate::assert_thread_safe::<librashader::runtime::gl::FilterChain>();
