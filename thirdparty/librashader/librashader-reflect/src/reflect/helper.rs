use crate::error::{SemanticsErrorKind, ShaderReflectError};

pub struct UboData {
    // id: u32,
    // descriptor_set: u32,
    pub binding: u32,
    pub size: u32,
}

pub struct TextureData<'a> {
    // id: u32,
    // descriptor_set: u32,
    pub name: &'a str,
    pub binding: u32,
}

// todo: might want to take these crate helpers out.

#[derive(Copy, Clone)]
pub enum SemanticErrorBlame {
    Vertex,
    Fragment,
}

impl SemanticErrorBlame {
    pub fn error(self, kind: SemanticsErrorKind) -> ShaderReflectError {
        match self {
            SemanticErrorBlame::Vertex => ShaderReflectError::VertexSemanticError(kind),
            SemanticErrorBlame::Fragment => ShaderReflectError::FragmentSemanticError(kind),
        }
    }
}
