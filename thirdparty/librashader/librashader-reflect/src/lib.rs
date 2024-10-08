//! Shader reflection and cross-compilation for librashader.
//!
//! ## Usage
//!
//! librashader-reflect requires the `type_alias_impl_trait` nightly feature. You should choose your
//! target shading language, and a compilation type.
//!
//! ```rust
//! #![feature(type_alias_impl_trait)]
//!
//! use std::error::Error;
//! use librashader_preprocess::ShaderSource;
//! use librashader_presets::ShaderPreset;
//! use librashader_reflect::back::{CompileReflectShader, FromCompilation};
//! use librashader_reflect::back::targets::SPIRV;
//! use librashader_reflect::front::{Glslang, ShaderInputCompiler, SpirvCompilation};
//! use librashader_reflect::reflect::cross::SpirvCross;
//! use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
//! use librashader_reflect::reflect::semantics::ShaderSemantics;
//! type Artifact = impl CompileReflectShader<SPIRV, SpirvCompilation, SpirvCross>;
//! type ShaderPassMeta = ShaderPassArtifact<Artifact>;
//!
//! // Compile single shader
//! pub fn compile_spirv(
//!         source: &ShaderSource,
//!     ) -> Result<Artifact, Box<dyn Error>>
//! {
//!     let compilation = SpirvCompilation::try_from(&source);
//!     let spirv = SPIRV::from_compilation(compilation)?;
//!     Ok(spirv)
//! }
//!
//! // Compile preset
//! pub fn compile_preset(preset: ShaderPreset) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), Box<dyn Error>>
//! {
//!     let (passes, semantics) = SPIRV::compile_preset_passes::<SpirvCompilation, SpirvCross, Box<dyn Error>>(
//!     preset.passes, &preset.textures)?;
//!     Ok((passes, semantics))
//! }
//! ```
//!
//! ## What's with all the traits?
//! librashader-reflect is designed to be compiler-agnostic. [naga](https://docs.rs/naga/latest/naga/index.html),
//! a pure-Rust shader compiler, as well as SPIRV-Cross via [SpirvCompilation](crate::front::SpirvCompilation)
//! is supported.
#![cfg_attr(not(feature = "stable"), feature(impl_trait_in_assoc_type))]
/// Shader codegen backends.
pub mod back;
/// Error types.
pub mod error;
/// Shader frontend parsers.
pub mod front;
/// Shader reflection.
pub mod reflect;
