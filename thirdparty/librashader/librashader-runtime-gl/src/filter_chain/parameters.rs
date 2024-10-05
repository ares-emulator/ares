use crate::filter_chain::chain::FilterChainImpl;
use crate::filter_chain::inner::FilterChainDispatch;
use crate::gl::GLInterface;
use crate::FilterChainGL;
use librashader_runtime::parameters::{FilterChainParameters, RuntimeParameters};

impl AsRef<dyn FilterChainParameters + 'static> for FilterChainDispatch {
    fn as_ref<'a>(&'a self) -> &'a (dyn FilterChainParameters + 'static) {
        match self {
            FilterChainDispatch::DirectStateAccess(p) => p,
            FilterChainDispatch::Compatibility(p) => p,
        }
    }
}

impl AsMut<dyn FilterChainParameters + 'static> for FilterChainDispatch {
    fn as_mut<'a>(&'a mut self) -> &'a mut (dyn FilterChainParameters + 'static) {
        match self {
            FilterChainDispatch::DirectStateAccess(p) => p,
            FilterChainDispatch::Compatibility(p) => p,
        }
    }
}

impl FilterChainParameters for FilterChainGL {
    fn parameters(&self) -> &RuntimeParameters {
        self.filter.as_ref().parameters()
    }
}

impl<T: GLInterface> FilterChainParameters for FilterChainImpl<T> {
    fn parameters(&self) -> &RuntimeParameters {
        &self.common.config
    }
}
