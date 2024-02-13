use crate::filter_chain::filter_impl::FilterChainImpl;
use crate::filter_chain::inner::FilterChainDispatch;
use crate::gl::GLInterface;
use crate::FilterChainGL;
use librashader_runtime::parameters::FilterChainParameters;
use std::collections::hash_map::Iter;

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
    fn get_enabled_pass_count(&self) -> usize {
        self.filter.as_ref().get_enabled_pass_count()
    }

    fn set_enabled_pass_count(&mut self, count: usize) {
        self.filter.as_mut().set_enabled_pass_count(count)
    }

    fn enumerate_parameters(&self) -> Iter<String, f32> {
        self.filter.as_ref().enumerate_parameters()
    }

    fn get_parameter(&self, parameter: &str) -> Option<f32> {
        self.filter.as_ref().get_parameter(parameter)
    }

    fn set_parameter(&mut self, parameter: &str, new_value: f32) -> Option<f32> {
        self.filter.as_mut().set_parameter(parameter, new_value)
    }
}

impl<T: GLInterface> FilterChainParameters for FilterChainImpl<T> {
    fn get_enabled_pass_count(&self) -> usize {
        self.common.config.passes_enabled
    }

    fn set_enabled_pass_count(&mut self, count: usize) {
        self.common.config.passes_enabled = count
    }

    fn enumerate_parameters(&self) -> Iter<String, f32> {
        self.common.config.parameters.iter()
    }

    fn get_parameter(&self, parameter: &str) -> Option<f32> {
        self.common
            .config
            .parameters
            .get::<str>(parameter.as_ref())
            .copied()
    }

    fn set_parameter(&mut self, parameter: &str, new_value: f32) -> Option<f32> {
        if let Some(value) = self
            .common
            .config
            .parameters
            .get_mut::<str>(parameter.as_ref())
        {
            let old = *value;
            *value = new_value;
            Some(old)
        } else {
            None
        }
    }
}
