use crate::back::targets::HLSL;
use crate::back::{CompileReflectShader, CompilerBackend, FromCompilation};
use crate::error::ShaderReflectError;
use crate::front::SpirvCompilation;
use crate::reflect::cross::hlsl::HlslReflect;
use crate::reflect::cross::{CompiledProgram, SpirvCross};

/// The HLSL shader model version to target.
pub use spirv_cross2::compile::hlsl::HlslShaderModel;

/// Buffer assignment information
#[derive(Debug, Clone)]
pub struct HlslBufferAssignment {
    /// The name of the buffer
    pub name: String,
    /// The id of the buffer
    pub id: u32,
}

/// Buffer assignment information
#[derive(Debug, Clone, Default)]
pub struct HlslBufferAssignments {
    /// Buffer assignment information for UBO
    pub ubo: Option<HlslBufferAssignment>,
    /// Buffer assignment information for Push
    pub push: Option<HlslBufferAssignment>,
}

impl HlslBufferAssignments {
    fn find_mangled_id(mangled_name: &str) -> Option<u32> {
        if !mangled_name.starts_with("_") {
            return None;
        }

        let Some(next_underscore) = mangled_name[1..].find("_") else {
            return None;
        };

        mangled_name[1..next_underscore + 1].parse().ok()
    }

    fn find_mangled_name(buffer_name: &str, uniform_name: &str, mangled_name: &str) -> bool {
        // name prependded
        if mangled_name[buffer_name.len()..].starts_with("_")
            && &mangled_name[buffer_name.len() + 1..] == uniform_name
        {
            return true;
        }
        false
    }

    // Check if the mangled name matches.
    pub fn contains_uniform(&self, uniform_name: &str, mangled_name: &str) -> bool {
        let is_likely_id_mangled = mangled_name.starts_with("_");
        if !mangled_name.ends_with(uniform_name) {
            return false;
        }

        if let Some(ubo) = &self.ubo {
            if is_likely_id_mangled {
                if let Some(id) = Self::find_mangled_id(mangled_name) {
                    if id == ubo.id {
                        return true;
                    }
                }
            }

            // name prependded
            if Self::find_mangled_name(&ubo.name, uniform_name, mangled_name) {
                return true;
            }
        }

        if let Some(push) = &self.push {
            if is_likely_id_mangled {
                if let Some(id) = Self::find_mangled_id(mangled_name) {
                    if id == push.id {
                        return true;
                    }
                }
            }

            // name prependded
            if Self::find_mangled_name(&push.name, uniform_name, mangled_name) {
                return true;
            }
        }

        // Sometimes SPIRV-cross will assign variables to "global"
        if Self::find_mangled_name("global", uniform_name, mangled_name) {
            return true;
        }

        false
    }
}

/// The context for a HLSL compilation via spirv-cross.
pub struct CrossHlslContext {
    /// The compiled HLSL program.
    pub artifact: CompiledProgram<spirv_cross2::targets::Hlsl>,
    pub vertex_buffers: HlslBufferAssignments,
    pub fragment_buffers: HlslBufferAssignments,
}

#[cfg(not(feature = "stable"))]
impl FromCompilation<SpirvCompilation, SpirvCross> for HLSL {
    type Target = HLSL;
    type Options = Option<HlslShaderModel>;
    type Context = CrossHlslContext;
    type Output = impl CompileReflectShader<Self::Target, SpirvCompilation, SpirvCross>;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        Ok(CompilerBackend {
            backend: HlslReflect::try_from(&compile)?,
        })
    }
}

#[cfg(feature = "stable")]
impl FromCompilation<SpirvCompilation, SpirvCross> for HLSL {
    type Target = HLSL;
    type Options = Option<HlslShaderModel>;
    type Context = CrossHlslContext;
    type Output = Box<dyn CompileReflectShader<Self::Target, SpirvCompilation, SpirvCross> + Send>;

    fn from_compilation(
        compile: SpirvCompilation,
    ) -> Result<CompilerBackend<Self::Output>, ShaderReflectError> {
        Ok(CompilerBackend {
            backend: Box::new(HlslReflect::try_from(&compile)?),
        })
    }
}

#[cfg(test)]
mod test {
    use crate::back::hlsl::HlslBufferAssignments;

    #[test]
    pub fn mangled_id_test() {
        assert_eq!(HlslBufferAssignments::find_mangled_id("_19_MVP"), Some(19));
        assert_eq!(HlslBufferAssignments::find_mangled_id("_19"), None);
        assert_eq!(HlslBufferAssignments::find_mangled_id("_19_"), Some(19));
        assert_eq!(HlslBufferAssignments::find_mangled_id("19_"), None);
        assert_eq!(HlslBufferAssignments::find_mangled_id("_19MVP"), None);
        assert_eq!(
            HlslBufferAssignments::find_mangled_id("_19_29_MVP"),
            Some(19)
        );
    }

    #[test]
    pub fn mangled_name_test() {
        assert!(HlslBufferAssignments::find_mangled_name(
            "params",
            "MVP",
            "params_MVP"
        ));
    }
}
