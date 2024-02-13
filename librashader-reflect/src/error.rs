use crate::reflect::semantics::UniformMemberBlock;
use thiserror::Error;

/// Error type for shader compilation.
#[non_exhaustive]
#[derive(Error, Debug)]
pub enum ShaderCompileError {
    /// Compile error from naga.
    #[cfg(feature = "unstable-naga")]
    #[error("shader")]
    NagaCompileError(Vec<naga::front::glsl::Error>),

    /// Compilation error from glslang.
    #[error("glslang")]
    GlslangError(#[from] glslang::error::GlslangError),

    /// Error when initializing the glslang compiler.
    #[error("glslang init")]
    CompilerInitError,

    /// Error when transpiling from spirv-cross.
    #[error("cross")]
    SpirvCrossCompileError(#[from] spirv_cross::ErrorCode),

    /// Error when transpiling from spirv-to-dxil
    #[cfg(all(target_os = "windows", feature = "dxil"))]
    #[error("spirv-to-dxil")]
    SpirvToDxilCompileError(#[from] spirv_to_dxil::SpirvToDxilError),

    /// Error when transpiling from naga
    #[cfg(all(feature = "wgsl", feature = "naga"))]
    #[error("naga-wgsl")]
    NagaWgslError(#[from] naga::back::wgsl::Error),

    /// Error when transpiling from naga
    #[cfg(feature = "naga")]
    #[error("naga-spv")]
    NagaSpvError(#[from] naga::back::spv::Error),

    /// Error when transpiling from naga
    #[cfg(all(feature = "naga", feature = "msl"))]
    #[error("naga-spv")]
    NagaMslError(#[from] naga::back::msl::Error),

    /// Error when transpiling from naga
    #[cfg(any(feature = "naga", feature = "wgsl"))]
    #[error("naga-wgsl")]
    NagaValidationError(#[from] naga::WithSpan<naga::valid::ValidationError>),
}

/// The error kind encountered when reflecting shader semantics.
#[derive(Debug)]
pub enum SemanticsErrorKind {
    /// The number of uniform buffers was invalid. Only one UBO is permitted.
    InvalidUniformBufferCount(usize),
    /// The number of push constant blocks was invalid. Only one push constant block is permitted.
    InvalidPushBufferCount(usize),
    /// The size of the push constant block was invalid.
    InvalidPushBufferSize(u32),
    /// The location of a varying was invalid.
    InvalidLocation(u32),
    /// The requested descriptor set was invalid. Only descriptor set 0 is available.
    InvalidDescriptorSet(u32),
    /// The number of inputs to the shader was invalid.
    InvalidInputCount(usize),
    /// The number of outputs declared was invalid.
    InvalidOutputCount(usize),
    /// Expected a binding but there was none.
    MissingBinding,
    /// The declared binding point was invalid.
    InvalidBinding(u32),
    /// The declared resource type was invalid.
    InvalidResourceType,
    /// The range of a struct member was invalid.
    InvalidRange(u32),
    /// The number of entry points in the shader was invalid.
    InvalidEntryPointCount(usize),
    /// The requested uniform or texture name was not provided semantics.
    UnknownSemantics(String),
    /// The type of the requested uniform was not compatible with the provided semantics.
    InvalidTypeForSemantic(String),
}

/// Error type for shader reflection.
#[non_exhaustive]
#[derive(Error, Debug)]
pub enum ShaderReflectError {
    /// Reflection error from spirv-cross.
    #[error("spirv")]
    SpirvCrossError(#[from] spirv_cross::ErrorCode),
    /// Error when validating vertex shader semantics.
    #[error("error when verifying vertex semantics")]
    VertexSemanticError(SemanticsErrorKind),
    /// Error when validating fragment shader semantics.
    #[error("error when verifying texture semantics")]
    FragmentSemanticError(SemanticsErrorKind),
    /// The vertex and fragment shader must have the same UBO binding location.
    #[error("vertex and fragment shader must have same binding")]
    MismatchedUniformBuffer { vertex: u32, fragment: u32 },
    /// The filter chain was found to be non causal. A pass tried to access the target output
    /// in the future.
    #[error("filter chain is non causal")]
    NonCausalFilterChain { pass: usize, target: usize },
    /// The offset of the given uniform did not match up in both the vertex and fragment shader.
    #[error("mismatched offset")]
    MismatchedOffset {
        semantic: String,
        expected: usize,
        received: usize,
        ty: UniformMemberBlock,
        pass: usize,
    },
    /// The size of the given uniform did not match up in both the vertex and fragment shader.
    #[error("mismatched component")]
    MismatchedSize {
        semantic: String,
        vertex: u32,
        fragment: u32,
        pass: usize,
    },
    /// The binding number is already in use.
    #[error("the binding is already in use")]
    BindingInUse(u32),
    /// Error when transpiling from naga
    #[cfg(feature = "naga")]
    #[error("naga-spv")]
    NagaInputError(#[from] naga::front::spv::Error),
}

#[cfg(feature = "unstable-naga")]
impl From<Vec<naga::front::glsl::Error>> for ShaderCompileError {
    fn from(err: Vec<naga::front::glsl::Error>) -> Self {
        ShaderCompileError::NagaCompileError(err)
    }
}
