/// Trait for filter chains that allow runtime reflection of shader parameters.
pub trait FilterChainParameters {
    /// Gets the number of shader passes enabled at runtime.
    fn get_enabled_pass_count(&self) -> usize;

    /// Sets the number of shader passes enabled at runtime.
    fn set_enabled_pass_count(&mut self, count: usize);

    /// Enumerates the active parameters as well as their values in the current filter chain.
    fn enumerate_parameters(&self) -> std::collections::hash_map::Iter<String, f32>;

    /// Get the value of the given parameter if present.
    fn get_parameter(&self, parameter: &str) -> Option<f32>;

    /// Set the value of the given parameter if present.
    ///
    /// Returns `None` if the parameter did not exist, or the old value if successful.
    fn set_parameter(&mut self, parameter: &str, new_value: f32) -> Option<f32>;
}
