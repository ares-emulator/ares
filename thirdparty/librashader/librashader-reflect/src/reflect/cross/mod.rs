pub mod glsl;
pub mod hlsl;
pub mod msl;

use crate::error::{SemanticsErrorKind, ShaderReflectError};
use crate::front::SpirvCompilation;
use crate::reflect::semantics::{
    BindingMeta, BindingStage, BufferReflection, MemberOffset, ShaderReflection, ShaderSemantics,
    TextureBinding, TextureSemanticMap, TextureSemantics, TextureSizeMeta, TypeInfo,
    UniformMemberBlock, UniqueSemanticMap, UniqueSemantics, ValidateTypeSemantics, VariableMeta,
    MAX_BINDINGS_COUNT, MAX_PUSH_BUFFER_SIZE,
};
use crate::reflect::{align_uniform_size, ReflectShader};
use std::fmt::Debug;
use std::ops::Deref;

use spirv_cross::spirv::{Ast, Decoration, Module, Resource, ShaderResources, Type};
use spirv_cross::ErrorCode;

use crate::reflect::helper::{SemanticErrorBlame, TextureData, UboData};

/// Reflect shaders under SPIRV-Cross semantics.
#[derive(Debug)]
pub struct SpirvCross;

// This is "probably" OK.
unsafe impl<T: Send + spirv_cross::spirv::Target> Send for CrossReflect<T>
where
    Ast<T>: spirv_cross::spirv::Compile<T>,
    Ast<T>: spirv_cross::spirv::Parse<T>,
{
}

pub(crate) struct CrossReflect<T>
where
    T: spirv_cross::spirv::Target,
    Ast<T>: spirv_cross::spirv::Compile<T>,
    Ast<T>: spirv_cross::spirv::Parse<T>,
{
    vertex: Ast<T>,
    fragment: Ast<T>,
}

///The output of the SPIR-V AST after compilation.
///
/// This output is immutable and can not be recompiled later.
pub struct CompiledAst<T: spirv_cross::spirv::Target>(Ast<T>);
impl<T: spirv_cross::spirv::Target> Deref for CompiledAst<T> {
    type Target = Ast<T>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

/// The compiled SPIR-V program after compilation.
pub struct CompiledProgram<T>
where
    T: spirv_cross::spirv::Target,
{
    pub vertex: CompiledAst<T>,
    pub fragment: CompiledAst<T>,
}

impl ValidateTypeSemantics<Type> for UniqueSemantics {
    fn validate_type(&self, ty: &Type) -> Option<TypeInfo> {
        let (Type::Float {
            ref array,
            vecsize,
            columns,
            ..
        }
        | Type::Int {
            ref array,
            vecsize,
            columns,
            ..
        }
        | Type::UInt {
            ref array,
            vecsize,
            columns,
            ..
        }) = *ty
        else {
            return None;
        };

        if !array.is_empty() {
            return None;
        }

        let valid = match self {
            UniqueSemantics::MVP => {
                matches!(ty, Type::Float { .. }) && vecsize == 4 && columns == 4
            }
            UniqueSemantics::FrameCount
            | UniqueSemantics::Rotation
            | UniqueSemantics::TotalSubFrames
            | UniqueSemantics::CurrentSubFrame => {
                matches!(ty, Type::UInt { .. }) && vecsize == 1 && columns == 1
            }
            UniqueSemantics::FrameDirection => {
                matches!(ty, Type::Int { .. }) && vecsize == 1 && columns == 1
            }
            UniqueSemantics::FloatParameter => {
                matches!(ty, Type::Float { .. }) && vecsize == 1 && columns == 1
            }
            _ => matches!(ty, Type::Float { .. }) && vecsize == 4 && columns == 1,
        };

        if valid {
            Some(TypeInfo {
                size: vecsize,
                columns,
            })
        } else {
            None
        }
    }
}

impl ValidateTypeSemantics<Type> for TextureSemantics {
    fn validate_type(&self, ty: &Type) -> Option<TypeInfo> {
        let Type::Float {
            ref array,
            vecsize,
            columns,
            ..
        } = *ty
        else {
            return None;
        };

        if !array.is_empty() {
            return None;
        }

        if vecsize == 4 && columns == 1 {
            Some(TypeInfo {
                size: vecsize,
                columns,
            })
        } else {
            None
        }
    }
}

impl<T> TryFrom<&SpirvCompilation> for CrossReflect<T>
where
    T: spirv_cross::spirv::Target,
    Ast<T>: spirv_cross::spirv::Compile<T>,
    Ast<T>: spirv_cross::spirv::Parse<T>,
{
    type Error = ShaderReflectError;

    fn try_from(value: &SpirvCompilation) -> Result<Self, Self::Error> {
        let vertex_module = Module::from_words(&value.vertex);
        let fragment_module = Module::from_words(&value.fragment);

        let vertex = Ast::parse(&vertex_module)?;
        let fragment = Ast::parse(&fragment_module)?;

        Ok(CrossReflect { vertex, fragment })
    }
}

impl<T> CrossReflect<T>
where
    T: spirv_cross::spirv::Target,
    Ast<T>: spirv_cross::spirv::Compile<T>,
    Ast<T>: spirv_cross::spirv::Parse<T>,
{
    fn validate(
        &self,
        vertex_res: &ShaderResources,
        fragment_res: &ShaderResources,
    ) -> Result<(), ShaderReflectError> {
        if !vertex_res.sampled_images.is_empty()
            || !vertex_res.storage_buffers.is_empty()
            || !vertex_res.subpass_inputs.is_empty()
            || !vertex_res.storage_images.is_empty()
            || !vertex_res.atomic_counters.is_empty()
        {
            return Err(ShaderReflectError::VertexSemanticError(
                SemanticsErrorKind::InvalidResourceType,
            ));
        }

        if !fragment_res.storage_buffers.is_empty()
            || !fragment_res.subpass_inputs.is_empty()
            || !fragment_res.storage_images.is_empty()
            || !fragment_res.atomic_counters.is_empty()
        {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidResourceType,
            ));
        }

        let vert_inputs = vertex_res.stage_inputs.len();
        if vert_inputs != 2 {
            return Err(ShaderReflectError::VertexSemanticError(
                SemanticsErrorKind::InvalidInputCount(vert_inputs),
            ));
        }

        let frag_outputs = fragment_res.stage_outputs.len();
        if frag_outputs != 1 {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidOutputCount(frag_outputs),
            ));
        }

        let fragment_location = self
            .fragment
            .get_decoration(fragment_res.stage_outputs[0].id, Decoration::Location)?;
        if fragment_location != 0 {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidLocation(fragment_location),
            ));
        }

        // Ensure that vertex attributes use location 0 and 1
        let vert_mask = vertex_res.stage_inputs.iter().try_fold(0, |mask, input| {
            Ok::<u32, ErrorCode>(
                mask | 1 << self.vertex.get_decoration(input.id, Decoration::Location)?,
            )
        })?;
        if vert_mask != 0x3 {
            return Err(ShaderReflectError::VertexSemanticError(
                SemanticsErrorKind::InvalidLocation(vert_mask),
            ));
        }

        if vertex_res.uniform_buffers.len() > 1 {
            return Err(ShaderReflectError::VertexSemanticError(
                SemanticsErrorKind::InvalidUniformBufferCount(vertex_res.uniform_buffers.len()),
            ));
        }

        if vertex_res.push_constant_buffers.len() > 1 {
            return Err(ShaderReflectError::VertexSemanticError(
                SemanticsErrorKind::InvalidPushBufferCount(vertex_res.push_constant_buffers.len()),
            ));
        }

        if fragment_res.uniform_buffers.len() > 1 {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidUniformBufferCount(fragment_res.uniform_buffers.len()),
            ));
        }

        if fragment_res.push_constant_buffers.len() > 1 {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidPushBufferCount(
                    fragment_res.push_constant_buffers.len(),
                ),
            ));
        }
        Ok(())
    }
}

impl<T> CrossReflect<T>
where
    T: spirv_cross::spirv::Target,
    Ast<T>: spirv_cross::spirv::Compile<T>,
    Ast<T>: spirv_cross::spirv::Parse<T>,
{
    fn get_ubo_data(
        ast: &Ast<T>,
        ubo: &Resource,
        blame: SemanticErrorBlame,
    ) -> Result<UboData, ShaderReflectError> {
        let descriptor_set = ast.get_decoration(ubo.id, Decoration::DescriptorSet)?;
        let binding = ast.get_decoration(ubo.id, Decoration::Binding)?;
        if binding >= MAX_BINDINGS_COUNT {
            return Err(blame.error(SemanticsErrorKind::InvalidBinding(binding)));
        }
        if descriptor_set != 0 {
            return Err(blame.error(SemanticsErrorKind::InvalidDescriptorSet(descriptor_set)));
        }

        let size = ast.get_declared_struct_size(ubo.base_type_id)?;
        Ok(UboData {
            // descriptor_set,
            // id: ubo.id,
            binding,
            size,
        })
    }

    fn get_push_size(
        ast: &Ast<T>,
        push: &Resource,
        blame: SemanticErrorBlame,
    ) -> Result<u32, ShaderReflectError> {
        let size = ast.get_declared_struct_size(push.base_type_id)?;
        if size > MAX_PUSH_BUFFER_SIZE {
            return Err(blame.error(SemanticsErrorKind::InvalidPushBufferSize(size)));
        }
        Ok(size)
    }

    fn reflect_buffer_range_metas(
        ast: &Ast<T>,
        resource: &Resource,
        pass_number: usize,
        semantics: &ShaderSemantics,
        meta: &mut BindingMeta,
        offset_type: UniformMemberBlock,
        blame: SemanticErrorBlame,
    ) -> Result<(), ShaderReflectError> {
        let ranges = ast.get_active_buffer_ranges(resource.id)?;
        for range in ranges {
            let name = ast.get_member_name(resource.base_type_id, range.index)?;
            let ubo_type = ast.get_type(resource.base_type_id)?;
            let range_type = match ubo_type {
                Type::Struct { member_types, .. } => {
                    let range_type = member_types
                        .get(range.index as usize)
                        .cloned()
                        .ok_or(blame.error(SemanticsErrorKind::InvalidRange(range.index)))?;
                    ast.get_type(range_type)?
                }
                _ => return Err(blame.error(SemanticsErrorKind::InvalidResourceType)),
            };

            if let Some(parameter) = semantics.uniform_semantics.get_unique_semantic(&name) {
                let Some(typeinfo) = parameter.semantics.validate_type(&range_type) else {
                    return Err(blame.error(SemanticsErrorKind::InvalidTypeForSemantic(name)));
                };

                match &parameter.semantics {
                    UniqueSemantics::FloatParameter => {
                        let offset = range.offset;
                        if let Some(meta) = meta.parameter_meta.get_mut(&name) {
                            if let Some(expected) = meta.offset.offset(offset_type)
                                && expected != offset
                            {
                                return Err(ShaderReflectError::MismatchedOffset {
                                    semantic: name,
                                    expected,
                                    received: offset,
                                    ty: offset_type,
                                    pass: pass_number,
                                });
                            }
                            if meta.size != typeinfo.size {
                                return Err(ShaderReflectError::MismatchedSize {
                                    semantic: name,
                                    vertex: meta.size,
                                    fragment: typeinfo.size,
                                    pass: pass_number,
                                });
                            }

                            *meta.offset.offset_mut(offset_type) = Some(offset);
                        } else {
                            meta.parameter_meta.insert(
                                name.clone(),
                                VariableMeta {
                                    id: name,
                                    offset: MemberOffset::new(offset, offset_type),
                                    size: typeinfo.size,
                                },
                            );
                        }
                    }
                    semantics => {
                        let offset = range.offset;
                        if let Some(meta) = meta.unique_meta.get_mut(semantics) {
                            if let Some(expected) = meta.offset.offset(offset_type)
                                && expected != offset
                            {
                                return Err(ShaderReflectError::MismatchedOffset {
                                    semantic: name,
                                    expected,
                                    received: offset,
                                    ty: offset_type,
                                    pass: pass_number,
                                });
                            }
                            if meta.size != typeinfo.size * typeinfo.columns {
                                return Err(ShaderReflectError::MismatchedSize {
                                    semantic: name,
                                    vertex: meta.size,
                                    fragment: typeinfo.size,
                                    pass: pass_number,
                                });
                            }

                            *meta.offset.offset_mut(offset_type) = Some(offset);
                        } else {
                            meta.unique_meta.insert(
                                *semantics,
                                VariableMeta {
                                    id: name,
                                    offset: MemberOffset::new(offset, offset_type),
                                    size: typeinfo.size * typeinfo.columns,
                                },
                            );
                        }
                    }
                }
            } else if let Some(texture) = semantics.uniform_semantics.get_texture_semantic(&name) {
                let Some(_typeinfo) = texture.semantics.validate_type(&range_type) else {
                    return Err(blame.error(SemanticsErrorKind::InvalidTypeForSemantic(name)));
                };

                if let TextureSemantics::PassOutput = texture.semantics {
                    if texture.index >= pass_number {
                        return Err(ShaderReflectError::NonCausalFilterChain {
                            pass: pass_number,
                            target: texture.index,
                        });
                    }
                }

                let offset = range.offset;
                if let Some(meta) = meta.texture_size_meta.get_mut(&texture) {
                    if let Some(expected) = meta.offset.offset(offset_type)
                        && expected != offset
                    {
                        return Err(ShaderReflectError::MismatchedOffset {
                            semantic: name,
                            expected,
                            received: offset,
                            ty: offset_type,
                            pass: pass_number,
                        });
                    }

                    meta.stage_mask.insert(match blame {
                        SemanticErrorBlame::Vertex => BindingStage::VERTEX,
                        SemanticErrorBlame::Fragment => BindingStage::FRAGMENT,
                    });

                    *meta.offset.offset_mut(offset_type) = Some(offset);
                } else {
                    meta.texture_size_meta.insert(
                        texture,
                        TextureSizeMeta {
                            offset: MemberOffset::new(offset, offset_type),
                            stage_mask: match blame {
                                SemanticErrorBlame::Vertex => BindingStage::VERTEX,
                                SemanticErrorBlame::Fragment => BindingStage::FRAGMENT,
                            },
                            id: name,
                        },
                    );
                }
            } else {
                return Err(blame.error(SemanticsErrorKind::UnknownSemantics(name)));
            }
        }
        Ok(())
    }

    fn reflect_ubos(
        &mut self,
        vertex_ubo: Option<&Resource>,
        fragment_ubo: Option<&Resource>,
    ) -> Result<Option<BufferReflection<u32>>, ShaderReflectError> {
        if let Some(vertex_ubo) = vertex_ubo {
            self.vertex
                .set_decoration(vertex_ubo.id, Decoration::Binding, 0)?;
        }

        if let Some(fragment_ubo) = fragment_ubo {
            self.fragment
                .set_decoration(fragment_ubo.id, Decoration::Binding, 0)?;
        }

        match (vertex_ubo, fragment_ubo) {
            (None, None) => Ok(None),
            (Some(vertex_ubo), Some(fragment_ubo)) => {
                let vertex_ubo =
                    Self::get_ubo_data(&self.vertex, vertex_ubo, SemanticErrorBlame::Vertex)?;
                let fragment_ubo =
                    Self::get_ubo_data(&self.fragment, fragment_ubo, SemanticErrorBlame::Fragment)?;
                if vertex_ubo.binding != fragment_ubo.binding {
                    return Err(ShaderReflectError::MismatchedUniformBuffer {
                        vertex: vertex_ubo.binding,
                        fragment: fragment_ubo.binding,
                    });
                }

                let size = std::cmp::max(vertex_ubo.size, fragment_ubo.size);
                Ok(Some(BufferReflection {
                    binding: vertex_ubo.binding,
                    size: align_uniform_size(size),
                    stage_mask: BindingStage::VERTEX | BindingStage::FRAGMENT,
                }))
            }
            (Some(vertex_ubo), None) => {
                let vertex_ubo =
                    Self::get_ubo_data(&self.vertex, vertex_ubo, SemanticErrorBlame::Vertex)?;
                Ok(Some(BufferReflection {
                    binding: vertex_ubo.binding,
                    size: align_uniform_size(vertex_ubo.size),
                    stage_mask: BindingStage::VERTEX,
                }))
            }
            (None, Some(fragment_ubo)) => {
                let fragment_ubo =
                    Self::get_ubo_data(&self.fragment, fragment_ubo, SemanticErrorBlame::Fragment)?;
                Ok(Some(BufferReflection {
                    binding: fragment_ubo.binding,
                    size: align_uniform_size(fragment_ubo.size),
                    stage_mask: BindingStage::FRAGMENT,
                }))
            }
        }
    }

    fn reflect_texture_metas(
        &self,
        texture: TextureData,
        pass_number: usize,
        semantics: &ShaderSemantics,
        meta: &mut BindingMeta,
    ) -> Result<(), ShaderReflectError> {
        let Some(semantic) = semantics
            .texture_semantics
            .get_texture_semantic(texture.name)
        else {
            return Err(
                SemanticErrorBlame::Fragment.error(SemanticsErrorKind::UnknownSemantics(
                    texture.name.to_string(),
                )),
            );
        };

        if semantic.semantics == TextureSemantics::PassOutput && semantic.index >= pass_number {
            return Err(ShaderReflectError::NonCausalFilterChain {
                pass: pass_number,
                target: semantic.index,
            });
        }

        meta.texture_meta.insert(
            semantic,
            TextureBinding {
                binding: texture.binding,
            },
        );
        Ok(())
    }

    fn reflect_texture<'a>(
        &'a self,
        texture: &'a Resource,
    ) -> Result<TextureData<'a>, ShaderReflectError> {
        let descriptor_set = self
            .fragment
            .get_decoration(texture.id, Decoration::DescriptorSet)?;
        let binding = self
            .fragment
            .get_decoration(texture.id, Decoration::Binding)?;
        if descriptor_set != 0 {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidDescriptorSet(descriptor_set),
            ));
        }
        if binding >= MAX_BINDINGS_COUNT {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidBinding(binding),
            ));
        }

        Ok(TextureData {
            // id: texture.id,
            // descriptor_set,
            name: &texture.name,
            binding,
        })
    }

    fn reflect_push_constant_buffer(
        &mut self,
        vertex_pcb: Option<&Resource>,
        fragment_pcb: Option<&Resource>,
    ) -> Result<Option<BufferReflection<Option<u32>>>, ShaderReflectError> {
        if let Some(vertex_pcb) = vertex_pcb {
            self.vertex
                .set_decoration(vertex_pcb.id, Decoration::Binding, 1)?;
        }

        if let Some(fragment_pcb) = fragment_pcb {
            self.fragment
                .set_decoration(fragment_pcb.id, Decoration::Binding, 1)?;
        }

        match (vertex_pcb, fragment_pcb) {
            (None, None) => Ok(None),
            (Some(vertex_push), Some(fragment_push)) => {
                let vertex_size =
                    Self::get_push_size(&self.vertex, vertex_push, SemanticErrorBlame::Vertex)?;
                let fragment_size = Self::get_push_size(
                    &self.fragment,
                    fragment_push,
                    SemanticErrorBlame::Fragment,
                )?;

                let size = std::cmp::max(vertex_size, fragment_size);

                Ok(Some(BufferReflection {
                    binding: None,
                    size: align_uniform_size(size),
                    stage_mask: BindingStage::VERTEX | BindingStage::FRAGMENT,
                }))
            }
            (Some(vertex_push), None) => {
                let vertex_size =
                    Self::get_push_size(&self.vertex, vertex_push, SemanticErrorBlame::Vertex)?;
                Ok(Some(BufferReflection {
                    binding: None,
                    size: align_uniform_size(vertex_size),
                    stage_mask: BindingStage::VERTEX,
                }))
            }
            (None, Some(fragment_push)) => {
                let fragment_size = Self::get_push_size(
                    &self.fragment,
                    fragment_push,
                    SemanticErrorBlame::Fragment,
                )?;
                Ok(Some(BufferReflection {
                    binding: None,
                    size: align_uniform_size(fragment_size),
                    stage_mask: BindingStage::FRAGMENT,
                }))
            }
        }
    }
}

impl<T> ReflectShader for CrossReflect<T>
where
    T: spirv_cross::spirv::Target,
    Ast<T>: spirv_cross::spirv::Compile<T>,
    Ast<T>: spirv_cross::spirv::Parse<T>,
{
    fn reflect(
        &mut self,
        pass_number: usize,
        semantics: &ShaderSemantics,
    ) -> Result<ShaderReflection, ShaderReflectError> {
        let vertex_res = self.vertex.get_shader_resources()?;
        let fragment_res = self.fragment.get_shader_resources()?;
        self.validate(&vertex_res, &fragment_res)?;

        let vertex_ubo = vertex_res.uniform_buffers.first();
        let fragment_ubo = fragment_res.uniform_buffers.first();

        let ubo = self.reflect_ubos(vertex_ubo, fragment_ubo)?;

        let vertex_push = vertex_res.push_constant_buffers.first();
        let fragment_push = fragment_res.push_constant_buffers.first();

        let push_constant = self.reflect_push_constant_buffer(vertex_push, fragment_push)?;

        let mut meta = BindingMeta::default();

        if let Some(ubo) = vertex_ubo {
            Self::reflect_buffer_range_metas(
                &self.vertex,
                ubo,
                pass_number,
                semantics,
                &mut meta,
                UniformMemberBlock::Ubo,
                SemanticErrorBlame::Vertex,
            )?;
        }

        if let Some(ubo) = fragment_ubo {
            Self::reflect_buffer_range_metas(
                &self.fragment,
                ubo,
                pass_number,
                semantics,
                &mut meta,
                UniformMemberBlock::Ubo,
                SemanticErrorBlame::Fragment,
            )?;
        }

        if let Some(push) = vertex_push {
            Self::reflect_buffer_range_metas(
                &self.vertex,
                push,
                pass_number,
                semantics,
                &mut meta,
                UniformMemberBlock::PushConstant,
                SemanticErrorBlame::Vertex,
            )?;
        }

        if let Some(push) = fragment_push {
            Self::reflect_buffer_range_metas(
                &self.fragment,
                push,
                pass_number,
                semantics,
                &mut meta,
                UniformMemberBlock::PushConstant,
                SemanticErrorBlame::Fragment,
            )?;
        }

        let mut ubo_bindings = 0u16;
        if vertex_ubo.is_some() || fragment_ubo.is_some() {
            ubo_bindings = 1 << ubo.as_ref().expect("UBOs should be present").binding;
        }

        for sampled_image in &fragment_res.sampled_images {
            let texture_data = self.reflect_texture(sampled_image)?;
            if ubo_bindings & (1 << texture_data.binding) != 0 {
                return Err(ShaderReflectError::BindingInUse(texture_data.binding));
            }
            ubo_bindings |= 1 << texture_data.binding;

            self.reflect_texture_metas(texture_data, pass_number, semantics, &mut meta)?;
        }

        Ok(ShaderReflection {
            ubo,
            push_constant,
            meta,
        })
    }
}

#[cfg(test)]
mod test {
    use crate::reflect::cross::CrossReflect;
    use crate::reflect::ReflectShader;
    use rustc_hash::FxHashMap;

    use crate::front::{Glslang, ShaderInputCompiler};
    use crate::reflect::semantics::{Semantic, ShaderSemantics, UniformSemantic, UniqueSemantics};
    use librashader_common::map::FastHashMap;
    use librashader_preprocess::ShaderSource;
    use spirv_cross::glsl;
    use spirv_cross::glsl::{CompilerOptions, Version};

    #[test]
    pub fn test_into() {
        let result = ShaderSource::load("../test/basic.slang").unwrap();
        let mut uniform_semantics: FastHashMap<String, UniformSemantic> = Default::default();

        for (_index, param) in result.parameters.iter().enumerate() {
            uniform_semantics.insert(
                param.1.id.clone(),
                UniformSemantic::Unique(Semantic {
                    semantics: UniqueSemantics::FloatParameter,
                    index: (),
                }),
            );
        }
        let spirv = Glslang::compile(&result).unwrap();
        let mut reflect = CrossReflect::<glsl::Target>::try_from(&spirv).unwrap();
        let _shader_reflection = reflect
            .reflect(
                0,
                &ShaderSemantics {
                    uniform_semantics,
                    texture_semantics: Default::default(),
                },
            )
            .unwrap();
        let mut opts = CompilerOptions::default();
        opts.version = Version::V4_60;
        opts.enable_420_pack_extension = false;
        // let compiled: ShaderCompilerOutput<String, CrossWgslContext> = <CrossReflect<glsl::Target> as CompileShader<WGSL>>::compile(reflect, Version::V3_30).unwrap();
        // // eprintln!("{shader_reflection:#?}");
        // eprintln!("{}", compiled.fragment)
        // let mut loader = rspirv::dr::Loader::new();
        // rspirv::binary::parse_words(spirv.fragment.as_binary(), &mut loader).unwrap();
        // let module = loader.module();
        // println!("{:#}", module.disassemble());
    }
}
