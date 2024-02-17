/// Trait for filter chains that allow runtime reflection of shader parameters.
pub trait FilterChainParameters {
    /// Gets the number of shader passes enabled at runtime.
    fn get_enabled_pass_count(&self) -> usize;

    /// Sets the number of shader passes enabled at runtime.
    fn set_enabled_pass_count(&mut self, count: usize);

    /// Enumerates the active parameters as well as their values in the current filter chain.
    fn enumerate_parameters<'a>(
        &'a self,
    ) -> ::librashader_common::map::halfbrown::Iter<String, f32>;

    /// Get the value of the given parameter if present.
    fn get_parameter(&self, parameter: &str) -> Option<f32>;

    /// Set the value of the given parameter if present.
    ///
    /// Returns `None` if the parameter did not exist, or the old value if successful.
    fn set_parameter(&mut self, parameter: &str, new_value: f32) -> Option<f32>;
}

#[macro_export]
macro_rules! impl_filter_chain_parameters {
    ($ty:ty) => {
        impl ::librashader_runtime::parameters::FilterChainParameters for $ty {
            fn get_enabled_pass_count(&self) -> usize {
                self.common.config.passes_enabled
            }

            fn set_enabled_pass_count(&mut self, count: usize) {
                self.common.config.passes_enabled = count
            }

            fn enumerate_parameters<'a>(
                &'a self,
            ) -> ::librashader_common::map::halfbrown::Iter<String, f32> {
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
    };
}
