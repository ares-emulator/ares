use crate::FilterChainVulkan;
use librashader_runtime::parameters::FilterChainParameters;
use std::collections::hash_map::Iter;

impl FilterChainParameters for FilterChainVulkan {
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
