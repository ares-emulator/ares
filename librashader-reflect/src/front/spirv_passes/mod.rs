pub mod link_input_outputs;
pub mod lower_samplers;

// Load SPIR-V as an rspirv module
pub(crate) fn load_module(words: &[u32]) -> rspirv::dr::Module {
    let mut loader = rspirv::dr::Loader::new();
    rspirv::binary::parse_words(words, &mut loader).unwrap();
    let module = loader.module();
    module
}
