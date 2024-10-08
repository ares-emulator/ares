#[doc(hidden)]
pub mod glsl;

#[doc(hidden)]
pub mod hlsl;

#[doc(hidden)]
pub mod msl;

use crate::error::{SemanticsErrorKind, ShaderReflectError};
use crate::front::SpirvCompilation;
use crate::reflect::helper::{SemanticErrorBlame, TextureData, UboData};
use crate::reflect::semantics::{
    BindingMeta, BindingStage, BufferReflection, MemberOffset, ShaderReflection, ShaderSemantics,
    TextureBinding, TextureSemanticMap, TextureSemantics, TextureSizeMeta, TypeInfo,
    UniformMemberBlock, UniqueSemanticMap, UniqueSemantics, ValidateTypeSemantics, VariableMeta,
    MAX_BINDINGS_COUNT, MAX_PUSH_BUFFER_SIZE,
};
use crate::reflect::{align_uniform_size, ReflectShader};
use librashader_common::map::ShortString;
use spirv_cross2::compile::CompiledArtifact;
use spirv_cross2::reflect::{
    AllResources, BitWidth, DecorationValue, Resource, Scalar, ScalarKind, TypeInner,
};
use spirv_cross2::spirv::Decoration;
use spirv_cross2::Compiler;
use spirv_cross2::Module;
use std::fmt::Debug;

/// Reflect shaders under SPIRV-Cross semantics.
///
/// SPIRV-Cross supports GLSL, HLSL, SPIR-V, and MSL targets.
#[derive(Debug)]
pub struct SpirvCross;

// todo: make this under a mutex
pub(crate) struct CrossReflect<T>
where
    T: spirv_cross2::compile::CompilableTarget,
{
    vertex: Compiler<T>,
    fragment: Compiler<T>,
}

/// The compiled SPIR-V program after compilation.
pub struct CompiledProgram<T>
where
    T: spirv_cross2::compile::CompilableTarget,
{
    pub vertex: CompiledArtifact<T>,
    pub fragment: CompiledArtifact<T>,
}

impl ValidateTypeSemantics<TypeInner<'_>> for UniqueSemantics {
    fn validate_type(&self, ty: &TypeInner) -> Option<TypeInfo> {
        let (TypeInner::Vector { .. } | TypeInner::Scalar { .. } | TypeInner::Matrix { .. }) = *ty
        else {
            return None;
        };

        match self {
            UniqueSemantics::MVP => {
                if matches!(ty, TypeInner::Matrix { columns, rows, scalar: Scalar { size, .. } } if *columns == 4
                    && *rows == 4 && *size == BitWidth::Word)
                {
                    return Some(TypeInfo {
                        size: 4,
                        columns: 4,
                    });
                }
            }
            UniqueSemantics::FrameCount
            | UniqueSemantics::Rotation
            | UniqueSemantics::CurrentSubFrame
            | UniqueSemantics::TotalSubFrames => {
                // Uint32 == width 4
                if matches!(ty, TypeInner::Scalar( Scalar { kind, size }) if *kind == ScalarKind::Uint && *size == BitWidth::Word)
                {
                    return Some(TypeInfo {
                        size: 1,
                        columns: 1,
                    });
                }
            }
            UniqueSemantics::FrameDirection => {
                // iint32 == width 4
                if matches!(ty, TypeInner::Scalar( Scalar { kind, size }) if *kind == ScalarKind::Int && *size == BitWidth::Word)
                {
                    return Some(TypeInfo {
                        size: 1,
                        columns: 1,
                    });
                }
            }
            UniqueSemantics::FloatParameter => {
                // Float32 == width 4
                if matches!(ty, TypeInner::Scalar( Scalar { kind, size }) if *kind == ScalarKind::Float && *size == BitWidth::Word)
                {
                    return Some(TypeInfo {
                        size: 1,
                        columns: 1,
                    });
                }
            }
            _ => {
                if matches!(ty, TypeInner::Vector { scalar: Scalar { size, kind }, width: vecwidth, .. }
                    if *kind == ScalarKind::Float && *size == BitWidth::Word && *vecwidth == 4)
                {
                    return Some(TypeInfo {
                        size: 4,
                        columns: 1,
                    });
                }
            }
        };

        None
    }
}

impl ValidateTypeSemantics<TypeInner<'_>> for TextureSemantics {
    fn validate_type(&self, ty: &TypeInner) -> Option<TypeInfo> {
        let TypeInner::Vector {
            scalar: Scalar { size, kind },
            width: vecwidth,
        } = ty
        else {
            return None;
        };

        if *kind == ScalarKind::Float && *size == BitWidth::Word && *vecwidth == 4 {
            return Some(TypeInfo {
                size: 4,
                columns: 1,
            });
        }

        None
    }
}

impl<T> TryFrom<&SpirvCompilation> for CrossReflect<T>
where
    T: spirv_cross2::compile::CompilableTarget,
{
    type Error = ShaderReflectError;

    fn try_from(value: &SpirvCompilation) -> Result<Self, Self::Error> {
        let vertex_module = Module::from_words(&value.vertex);
        let fragment_module = Module::from_words(&value.fragment);

        let vertex = Compiler::new(vertex_module)?;
        let fragment = Compiler::new(fragment_module)?;

        Ok(CrossReflect { vertex, fragment })
    }
}

impl<T> CrossReflect<T>
where
    T: spirv_cross2::compile::CompilableTarget,
{
    fn validate_semantics(
        &self,
        vertex_res: &AllResources,
        fragment_res: &AllResources,
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

        let Some(DecorationValue::Literal(fragment_location)) = self
            .fragment
            .decoration(fragment_res.stage_outputs[0].id, Decoration::Location)?
        else {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::MissingBinding,
            ));
        };

        if fragment_location != 0 {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidLocation(fragment_location),
            ));
        }

        // Ensure that vertex attributes use location 0 and 1
        // Verify Vertex inputs
        'vertex: {
            let entry_points = self.vertex.entry_points()?;
            if entry_points.len() != 1 {
                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::InvalidEntryPointCount(entry_points.len()),
                ));
            }

            let vert_inputs = vertex_res.stage_inputs.len();
            if vert_inputs != 2 {
                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::InvalidInputCount(vert_inputs),
                ));
            }

            for input in &vertex_res.stage_inputs {
                let location = self.vertex.decoration(input.id, Decoration::Location)?;
                let Some(DecorationValue::Literal(location)) = location else {
                    return Err(ShaderReflectError::VertexSemanticError(
                        SemanticsErrorKind::MissingBinding,
                    ));
                };

                if location == 0 {
                    let pos_type = &self.vertex.type_description(input.base_type_id)?;
                    if !matches!(pos_type.inner, TypeInner::Vector { width, ..} if width == 4) {
                        return Err(ShaderReflectError::VertexSemanticError(
                            SemanticsErrorKind::InvalidLocation(location),
                        ));
                    }
                    break 'vertex;
                }

                if location == 1 {
                    let coord_type = &self.vertex.type_description(input.base_type_id)?;
                    if !matches!(coord_type.inner, TypeInner::Vector { width, ..} if width == 2) {
                        return Err(ShaderReflectError::VertexSemanticError(
                            SemanticsErrorKind::InvalidLocation(location),
                        ));
                    }
                    break 'vertex;
                }

                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::InvalidLocation(location),
                ));
            }
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
    T: spirv_cross2::compile::CompilableTarget,
{
    fn get_ubo_data(
        ast: &Compiler<T>,
        ubo: &Resource,
        blame: SemanticErrorBlame,
    ) -> Result<UboData, ShaderReflectError> {
        let Some(descriptor_set) = ast
            .decoration(ubo.id, Decoration::DescriptorSet)?
            .and_then(|l| l.as_literal())
        else {
            return Err(blame.error(SemanticsErrorKind::MissingBinding));
        };

        let Some(binding) = ast
            .decoration(ubo.id, Decoration::Binding)?
            .and_then(|l| l.as_literal())
        else {
            return Err(blame.error(SemanticsErrorKind::MissingBinding));
        };

        if binding >= MAX_BINDINGS_COUNT {
            return Err(blame.error(SemanticsErrorKind::InvalidBinding(binding)));
        }
        if descriptor_set != 0 {
            return Err(blame.error(SemanticsErrorKind::InvalidDescriptorSet(descriptor_set)));
        }

        let size = ast.type_description(ubo.base_type_id)?.size_hint.declared() as u32;
        Ok(UboData { binding, size })
    }

    fn get_push_size(
        ast: &Compiler<T>,
        push: &Resource,
        blame: SemanticErrorBlame,
    ) -> Result<u32, ShaderReflectError> {
        let size = ast
            .type_description(push.base_type_id)?
            .size_hint
            .declared() as u32;
        if size > MAX_PUSH_BUFFER_SIZE {
            return Err(blame.error(SemanticsErrorKind::InvalidPushBufferSize(size)));
        }
        Ok(size)
    }

    fn reflect_buffer_range_metas(
        ast: &Compiler<T>,
        resource: &Resource,
        pass_number: usize,
        semantics: &ShaderSemantics,
        meta: &mut BindingMeta,
        offset_type: UniformMemberBlock,
        blame: SemanticErrorBlame,
    ) -> Result<(), ShaderReflectError> {
        let ranges = ast.active_buffer_ranges(resource.id)?;
        for range in ranges {
            let Some(name) = ast.member_name(resource.base_type_id, range.index)? else {
                // member has no name!
                return Err(blame.error(SemanticsErrorKind::InvalidRange(range.index)));
            };

            let ubo_type = ast.type_description(resource.base_type_id)?;
            let range_type = match ubo_type.inner {
                TypeInner::Struct(struct_def) => {
                    let range_type = struct_def
                        .members
                        .get(range.index as usize)
                        .ok_or(blame.error(SemanticsErrorKind::InvalidRange(range.index)))?;
                    ast.type_description(range_type.id)?
                }
                _ => return Err(blame.error(SemanticsErrorKind::InvalidResourceType)),
            };

            if let Some(parameter) = semantics.uniform_semantics.unique_semantic(&name) {
                let Some(typeinfo) = parameter.semantics.validate_type(&range_type.inner) else {
                    return Err(
                        blame.error(SemanticsErrorKind::InvalidTypeForSemantic(name.to_string()))
                    );
                };

                match &parameter.semantics {
                    UniqueSemantics::FloatParameter => {
                        let offset = range.offset;
                        if let Some(meta) = meta.parameter_meta.get_mut::<str>(&name.as_ref()) {
                            if let Some(expected) = meta
                                .offset
                                .offset(offset_type)
                                .filter(|expected| *expected != offset)
                            {
                                return Err(ShaderReflectError::MismatchedOffset {
                                    semantic: name.to_string(),
                                    expected,
                                    received: offset,
                                    ty: offset_type,
                                    pass: pass_number,
                                });
                            }
                            if meta.size != typeinfo.size {
                                return Err(ShaderReflectError::MismatchedSize {
                                    semantic: name.to_string(),
                                    vertex: meta.size,
                                    fragment: typeinfo.size,
                                    pass: pass_number,
                                });
                            }

                            *meta.offset.offset_mut(offset_type) = Some(offset);
                        } else {
                            let name = ShortString::from(name.as_ref());
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
                            if let Some(expected) = meta
                                .offset
                                .offset(offset_type)
                                .filter(|expected| *expected != offset)
                            {
                                return Err(ShaderReflectError::MismatchedOffset {
                                    semantic: name.to_string(),
                                    expected,
                                    received: offset,
                                    ty: offset_type,
                                    pass: pass_number,
                                });
                            }
                            if meta.size != typeinfo.size * typeinfo.columns {
                                return Err(ShaderReflectError::MismatchedSize {
                                    semantic: name.to_string(),
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
                                    id: ShortString::from(name.as_ref()),
                                    offset: MemberOffset::new(offset, offset_type),
                                    size: typeinfo.size * typeinfo.columns,
                                },
                            );
                        }
                    }
                }
            } else if let Some(texture) = semantics.uniform_semantics.texture_semantic(&name) {
                let Some(_typeinfo) = texture.semantics.validate_type(&range_type.inner) else {
                    return Err(
                        blame.error(SemanticsErrorKind::InvalidTypeForSemantic(name.to_string()))
                    );
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
                    if let Some(expected) = meta
                        .offset
                        .offset(offset_type)
                        .filter(|expected| *expected != offset)
                    {
                        return Err(ShaderReflectError::MismatchedOffset {
                            semantic: name.to_string(),
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
                            id: ShortString::from(name.as_ref()),
                        },
                    );
                }
            } else {
                return Err(blame.error(SemanticsErrorKind::UnknownSemantics(name.to_string())));
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
                .set_decoration(vertex_ubo.id, Decoration::Binding, Some(0))?;
        }

        if let Some(fragment_ubo) = fragment_ubo {
            self.fragment
                .set_decoration(fragment_ubo.id, Decoration::Binding, Some(0))?;
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
        let Some(semantic) = semantics.texture_semantics.texture_semantic(texture.name) else {
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
        let Some(descriptor_set) = self
            .fragment
            .decoration(texture.id, Decoration::DescriptorSet)?
            .and_then(|l| l.as_literal())
        else {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::MissingBinding,
            ));
        };
        let Some(binding) = self
            .fragment
            .decoration(texture.id, Decoration::Binding)?
            .and_then(|l| l.as_literal())
        else {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::MissingBinding,
            ));
        };

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
                .set_decoration(vertex_pcb.id, Decoration::Binding, Some(1))?;
        }

        if let Some(fragment_pcb) = fragment_pcb {
            self.fragment
                .set_decoration(fragment_pcb.id, Decoration::Binding, Some(1))?;
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
    T: spirv_cross2::compile::CompilableTarget,
{
    fn reflect(
        &mut self,
        pass_number: usize,
        semantics: &ShaderSemantics,
    ) -> Result<ShaderReflection, ShaderReflectError> {
        let vertex_res = self.vertex.shader_resources()?.all_resources()?;
        let fragment_res = self.fragment.shader_resources()?.all_resources()?;
        self.validate_semantics(&vertex_res, &fragment_res)?;

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

    fn validate(&mut self) -> Result<(), ShaderReflectError> {
        let vertex_res = self.vertex.shader_resources()?.all_resources()?;
        let fragment_res = self.fragment.shader_resources()?.all_resources()?;
        self.validate_semantics(&vertex_res, &fragment_res)?;
        let vertex_ubo = vertex_res.uniform_buffers.first();
        let fragment_ubo = fragment_res.uniform_buffers.first();

        self.reflect_ubos(vertex_ubo, fragment_ubo)?;

        let vertex_push = vertex_res.push_constant_buffers.first();
        let fragment_push = fragment_res.push_constant_buffers.first();

        self.reflect_push_constant_buffer(vertex_push, fragment_push)?;

        Ok(())
    }
}

#[cfg(test)]
mod test {
    use crate::reflect::cross::CrossReflect;
    use crate::reflect::ReflectShader;
    use rustc_hash::FxHashMap;

    use crate::back::hlsl::CrossHlslContext;
    use crate::back::targets::HLSL;
    use crate::back::{CompileShader, ShaderCompilerOutput};
    use crate::front::{Glslang, ShaderInputCompiler};
    use crate::reflect::semantics::{Semantic, ShaderSemantics, UniformSemantic, UniqueSemantics};
    use librashader_common::map::{FastHashMap, ShortString};
    use librashader_preprocess::ShaderSource;

    // #[test]
    // pub fn test_into() {
    //     let result = ShaderSource::load("../test/basic.slang").unwrap();
    //     let mut uniform_semantics: FastHashMap<ShortString, UniformSemantic> = Default::default();
    //
    //     for (_index, param) in result.parameters.iter().enumerate() {
    //         uniform_semantics.insert(
    //             param.1.id.clone(),
    //             UniformSemantic::Unique(Semantic {
    //                 semantics: UniqueSemantics::FloatParameter,
    //                 index: (),
    //             }),
    //         );
    //     }
    //     let spirv = Glslang::compile(&result).unwrap();
    //     let mut reflect = CrossReflect::<hlsl::Target>::try_from(&spirv).unwrap();
    //     let shader_reflection = reflect
    //         .reflect(
    //             0,
    //             &ShaderSemantics {
    //                 uniform_semantics,
    //                 texture_semantics: Default::default(),
    //             },
    //         )
    //         .unwrap();
    //     let mut opts = hlsl::CompilerOptions::default();
    //     opts.shader_model = ShaderModel::V3_0;
    //
    //     let compiled: ShaderCompilerOutput<String, CrossHlslContext> =
    //         <CrossReflect<hlsl::Target> as CompileShader<HLSL>>::compile(
    //             reflect,
    //             Some(ShaderModel::V3_0),
    //         )
    //         .unwrap();
    //
    //     println!("{:?}", shader_reflection.meta);
    //     println!("{}", compiled.fragment);
    //     println!("{}", compiled.vertex);
    //
    //     // // eprintln!("{shader_reflection:#?}");
    //     // eprintln!("{}", compiled.fragment)
    //     // let mut loader = rspirv::dr::Loader::new();
    //     // rspirv::binary::parse_words(spirv.fragment.as_binary(), &mut loader).unwrap();
    //     // let module = loader.module();
    //     // println!("{:#}", module.disassemble());
    // }
}
