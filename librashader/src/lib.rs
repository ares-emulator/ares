#![forbid(missing_docs)]
#![cfg_attr(feature = "docsrs", feature(doc_cfg))]
//! RetroArch shader preset compiler and runtime.
//!
//! librashader provides convenient and safe access to RetroArch ['slang' shaders](https://github.com/libretro/slang-shaders).
//! The preset parser, shader preprocessor, and shader runtimes have all been reimplemented in Rust to provide easy access to
//! the rich library of shaders.
//!
//! ## Usage
//! The core objects in librashader are the [`ShaderPreset`](crate::presets::ShaderPreset) and the
//! filter chain implementations.
//!
//! The basic workflow involves parsing a `ShaderPreset`, which can then be used to construct
//! a `FilterChain`. All shaders will then be compiled, after which `FilterChain::frame` can be
//! called with appropriate input and output parameters to draw a frame with the shader effect applied.
//!
//! ## Runtimes
//! librashader supports most modern graphics runtimes, including Vulkan, OpenGL 3.3+ and 4.6 (with DSA),
//! Direct3D 11, Direct3D 12, and Metal. Secondary support is available for wgpu and Direct3D 9.
//!
//! The Direct3D 12 runtime requires support for [render passes](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-render-passes), which
//! have been available since Windows 10, version 1809.
//!
//! The Vulkan runtime can use [`VK_KHR_dynamic_rendering`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_dynamic_rendering.html)
//! for improved performance, if the underlying hardware supports it.
//!
//! Shader compatibility is not guaranteed on render APIs with secondary support.
//!
//! wgpu has restrictions on shaders that can not be converted to WGSL, such as those that use `inverse`.
//! Direct3D 9 does not support shaders that need Direct3D 10+ only features, or shaders that can not be
//! compiled to [Shader Model 3.0](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/shader-model-3).
//!
//! wgpu support is not available in the librashader C API.
//!
//! | **API**     | **Status** | **`librashader` feature** |
//! |-------------|-----------|---------------------------|
//! | OpenGL 3.3+ | ✅        | `gl`                      |
//! | OpenGL 4.6  | ✅        | `gl`                      |
//! | Vulkan      | ✅        | `vk`                      |
//! | Direct3D 9  | 🆗️        | `d3d9`                   |
//! | Direct3D 11 | ✅        | `d3d11`                   |
//! | Direct3D 12 | ✅        | `d3d12`                   |
//! | Metal       | ✅        | `metal`                   |
//! | wgpu        | 🆗         | `wgpu`                    |
//!
//! ✅ Full Support &mdash; 🆗 Secondary Support
//!
//! ## C API
//! For documentation on the librashader C API, see [librashader-capi](https://docs.rs/librashader-capi/latest/librashader_capi/),
//! or [`librashader.h`](https://github.com/SnowflakePowered/librashader/blob/master/include/librashader.h).

pub use librashader_common::map::FastHashMap;
pub use librashader_common::map::ShortString;

#[cfg(feature = "presets")]
#[cfg_attr(feature = "docsrs", doc(cfg(feature = "presets")))]
/// Parsing and usage of shader presets.
///
/// This module contains facilities and types for parsing `.slangp` shader presets files.
///
/// Shader presets contain shader and texture parameters, and the order in which to apply a set of
/// shaders in a filter chain. A librashader runtime takes a resulting [`ShaderPreset`](crate::presets::ShaderPreset)
/// as input to create a filter chain.
///
/// librashader's preset parser has been tested against all presets in the [slang-shaders](https://github.com/libretro/slang-shaders) repository
/// to generally good compatibility. However, the preset parser requires all referenced paths to resolve to a canonical, existing path relative
/// to the preset file. The handful of shaders that fail to parse due to this or other reasons are
/// listed at [`BROKEN_SHADERS.md`](https://github.com/SnowflakePowered/librashader/blob/master/BROKEN_SHADERS.md).
pub mod presets {
    use librashader_preprocess::{PreprocessError, ShaderParameter, ShaderSource};
    pub use librashader_presets::*;

    pub use librashader_pack::*;

    /// Get full parameter metadata from a shader preset.
    pub fn get_parameter_meta(
        preset: &ShaderPreset,
    ) -> Result<impl Iterator<Item = ShaderParameter>, PreprocessError> {
        let iters: Result<Vec<Vec<ShaderParameter>>, PreprocessError> = preset
            .passes
            .iter()
            .map(|s| {
                ShaderSource::load(&s.path)
                    .map(|s| s.parameters.into_iter().map(|(_, v)| v).collect())
            })
            .collect();
        let iters = iters?;
        Ok(iters.into_iter().flatten())
    }
}

#[cfg(feature = "preprocess")]
#[cfg_attr(feature = "docsrs", doc(cfg(feature = "preprocess")))]
/// Loading and preprocessing of 'slang' shader source files.
///
/// This module contains facilities and types for resolving `#include` directives in `.slang`
/// into a single compilation unit. `#pragma` directives are also parsed and resolved as
/// [`ShaderParameter`](crate::preprocess::ShaderParameter) structs.
///
/// The resulting [`ShaderSource`](crate::preprocess::ShaderSource) can then be passed into a
/// reflection target for reflection and compilation into the target shader format.
pub mod preprocess {
    pub use librashader_preprocess::*;
}

#[cfg(feature = "reflect")]
#[cfg_attr(feature = "docsrs", doc(cfg(feature = "reflect")))]
/// Shader reflection and cross-compilation.
///
/// Without the `stable` crate feature, the `type_alias_impl_trait` nightly feature is required.
///
/// You should choose your target shading language, and a compilation type.
///
/// ```rust
/// #![feature(type_alias_impl_trait)]
/// mod compile {
///     use std::error::Error;
///     use librashader::preprocess::ShaderSource;
///     use librashader::presets::ShaderPreset;
///     use librashader::reflect::{CompileReflectShader, FromCompilation, CompilePresetTarget, ShaderPassArtifact};
///     use librashader::reflect::targets::SPIRV;
///     use librashader::reflect::SpirvCompilation;
///     use librashader::reflect::cross::SpirvCross;
///     use librashader::reflect::semantics::ShaderSemantics;
///
///     type Artifact = impl CompileReflectShader<SPIRV, SpirvCompilation, SpirvCross>;
///     type ShaderPassMeta = ShaderPassArtifact<Artifact>;
///
///      // Compile preset
///     pub fn compile_preset(preset: ShaderPreset) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), Box<dyn Error>>
///     {
///         let (passes, semantics) = SPIRV::compile_preset_passes::<SpirvCompilation, SpirvCross, Box<dyn Error>>(
///         preset.passes, &preset.textures)?;
///         Ok((passes, semantics))
///     }
/// }
/// ```
///
/// With the `stable` crate feature, a trait object can be used instead. Note the `Send` bound
/// on `Artifact` is required.
///
/// ```
/// use librashader::reflect::CompileReflectShader;
/// use librashader::reflect::targets::SPIRV;
/// use librashader::reflect::SpirvCompilation;
/// use librashader::reflect::cross::SpirvCross;
/// use librashader::reflect::ShaderPassArtifact;
///
/// type Artifact = Box<dyn CompileReflectShader<SPIRV, SpirvCompilation, SpirvCross> + Send>;
/// type ShaderPassMeta = ShaderPassArtifact<Artifact>;
/// ```
///
/// ## What's with all the traits?
/// librashader-reflect is designed to be compiler-agnostic. [naga](https://docs.rs/naga/latest/naga/index.html),
/// a pure-Rust shader compiler, as well as SPIRV-Cross via [SpirvCompilation](crate::front::SpirvCompilation)
/// is supported.
pub mod reflect {
    /// Supported shader compiler targets.
    pub mod targets {
        pub use librashader_reflect::back::targets::DXIL;
        pub use librashader_reflect::back::targets::GLSL;
        pub use librashader_reflect::back::targets::HLSL;
        pub use librashader_reflect::back::targets::MSL;
        pub use librashader_reflect::back::targets::SPIRV;
        pub use librashader_reflect::back::targets::WGSL;
    }

    pub use librashader_reflect::error::*;

    pub use librashader_reflect::reflect::{semantics, ReflectShader, ShaderReflection};

    pub use librashader_reflect::back::{
        targets::OutputTarget, CompileReflectShader, CompileShader, CompilerBackend,
        FromCompilation, ShaderCompilerOutput,
    };

    pub use librashader_reflect::front::{ShaderReflectObject, SpirvCompilation};

    /// Reflection via SPIRV-Cross.
    #[cfg(feature = "reflect-cross")]
    #[cfg_attr(feature = "docsrs", doc(cfg(feature = "reflect-cross")))]
    pub mod cross {
        pub use librashader_reflect::reflect::cross::SpirvCross;

        /// The version of GLSL to target.
        ///
        pub use librashader_reflect::back::glsl::GlslVersion;

        /// The HLSL Shader Model to target.
        ///
        pub use librashader_reflect::back::hlsl::HlslShaderModel;

        /// The MSL version to target.
        ///
        pub use librashader_reflect::back::msl::MslVersion;

        pub use librashader_reflect::back::glsl::CrossGlslContext;

        pub use librashader_reflect::back::hlsl::CrossHlslContext;

        pub use librashader_reflect::back::msl::CrossMslContext;

        pub use librashader_reflect::reflect::cross::CompiledProgram;
    }

    /// DXIL reflection via spirv-to-dxil.
    #[cfg(all(target_os = "windows", feature = "reflect-dxil"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_os = "windows", feature = "reflect-dxil")))
    )]
    pub mod dxil {
        /// The maximum shader model to use when compiling the DXIL blob.
        pub use librashader_reflect::back::dxil::ShaderModel;

        /// A compiled DXIL artifact.
        pub use librashader_reflect::back::dxil::DxilObject;
    }

    /// Reflection via Naga
    #[cfg(feature = "reflect-naga")]
    #[cfg_attr(feature = "docsrs", doc(cfg(feature = "reflect-naga")))]
    pub mod naga {
        pub use librashader_reflect::back::wgsl::NagaWgslContext;
        pub use librashader_reflect::reflect::naga::Naga;
        pub use librashader_reflect::reflect::naga::NagaLoweringOptions;
    }

    pub use librashader_reflect::reflect::semantics::BindingMeta;

    pub use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};

    pub use librashader_reflect::front::ShaderInputCompiler;

    #[doc(hidden)]
    #[cfg(feature = "internal")]
    /// Helper methods for runtimes.
    ///
    /// This is internal to librashader runtimes and is exempt from semantic versioning.
    pub mod helper {
        pub use librashader_runtime::image;
    }
}

/// Shader runtimes to execute a filter chain on a GPU surface.
#[cfg(feature = "runtime")]
#[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime")))]
pub mod runtime {
    pub use librashader_common::{Size, Viewport};
    pub use librashader_runtime::parameters::FilterChainParameters;
    pub use librashader_runtime::parameters::RuntimeParameters;

    #[cfg(feature = "runtime-gl")]
    #[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime-gl")))]
    /// Shader runtime for OpenGL 3.3+.
    ///
    /// DSA support requires OpenGL 4.6.
    pub mod gl {
        pub use librashader_runtime_gl::{
            error,
            options::{FilterChainOptionsGL as FilterChainOptions, FrameOptionsGL as FrameOptions},
            FilterChainGL as FilterChain, GLImage,
        };
    }

    #[cfg(all(target_os = "windows", feature = "runtime-d3d11"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_os = "windows", feature = "runtime-d3d11")))
    )]
    /// Shader runtime for Direct3D 11.
    pub mod d3d11 {
        pub use librashader_runtime_d3d11::{
            error,
            options::{
                FilterChainOptionsD3D11 as FilterChainOptions, FrameOptionsD3D11 as FrameOptions,
            },
            FilterChainD3D11 as FilterChain,
        };
    }

    #[cfg(all(target_os = "windows", feature = "runtime-d3d12"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_os = "windows", feature = "runtime-d3d12")))
    )]
    /// Shader runtime for Direct3D 12.
    pub mod d3d12 {
        pub use librashader_runtime_d3d12::{
            error,
            options::{
                FilterChainOptionsD3D12 as FilterChainOptions, FrameOptionsD3D12 as FrameOptions,
            },
            D3D12InputImage, D3D12OutputView, FilterChainD3D12 as FilterChain,
        };
    }

    #[cfg(all(target_os = "windows", feature = "runtime-d3d9"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_os = "windows", feature = "runtime-d3d9")))
    )]
    /// Shader runtime for Direct3D 9.
    pub mod d3d9 {
        pub use librashader_runtime_d3d9::{
            error,
            options::{
                FilterChainOptionsD3D9 as FilterChainOptions, FrameOptionsD3D9 as FrameOptions,
            },
            FilterChainD3D9 as FilterChain,
        };
    }

    #[cfg(feature = "runtime-vk")]
    #[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime-vk")))]
    /// Shader runtime for Vulkan.
    pub mod vk {
        pub use librashader_runtime_vk::{
            error,
            options::{
                FilterChainOptionsVulkan as FilterChainOptions, FrameOptionsVulkan as FrameOptions,
            },
            FilterChainVulkan as FilterChain, VulkanImage, VulkanInstance, VulkanObjects,
        };
    }

    #[cfg(all(target_vendor = "apple", feature = "runtime-metal"))]
    #[cfg_attr(
        feature = "docsrs",
        doc(cfg(all(target_vendor = "apple", feature = "runtime-metal")))
    )]
    /// Shader runtime for Metal.
    pub mod mtl {
        pub use librashader_runtime_mtl::{
            error,
            options::{
                FilterChainOptionsMetal as FilterChainOptions, FrameOptionsMetal as FrameOptions,
            },
            FilterChainMetal as FilterChain, MetalTextureRef,
        };
    }

    #[cfg(feature = "runtime-wgpu")]
    #[cfg_attr(feature = "docsrs", doc(cfg(feature = "runtime-wgpu")))]
    /// Shader runtime for wgpu.
    pub mod wgpu {
        pub use librashader_runtime_wgpu::{
            error,
            options::{
                FilterChainOptionsWgpu as FilterChainOptions, FrameOptionsWgpu as FrameOptions,
            },
            FilterChainWgpu as FilterChain, WgpuOutputView,
        };
    }
}

pub use librashader_common::{FilterMode, ImageFormat, WrapMode};
