use crate::filter_chain::filter_impl::FilterChainImpl;

pub(in crate::filter_chain) enum FilterChainDispatch {
    DirectStateAccess(FilterChainImpl<crate::gl::gl46::DirectStateAccessGL>),
    Compatibility(FilterChainImpl<crate::gl::gl3::CompatibilityGL>),
}
