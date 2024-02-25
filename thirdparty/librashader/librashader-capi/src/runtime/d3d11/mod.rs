//! C API for the librashader D3D12 Runtime (`libra_d3d11_*`).

mod filter_chain;
pub use filter_chain::*;
const _: () = crate::assert_thread_safe::<librashader::runtime::d3d11::FilterChain>();
