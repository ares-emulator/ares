pub mod msl;
pub mod spirv;
mod spirv_passes;
pub mod wgsl;

use crate::error::{SemanticsErrorKind, ShaderReflectError};

use crate::front::SpirvCompilation;
use naga::{
    AddressSpace, Binding, Expression, GlobalVariable, Handle, ImageClass, Module, ResourceBinding,
    Scalar, ScalarKind, StructMember, TypeInner, VectorSize,
};
use rspirv::binary::Assemble;
use rspirv::dr::Builder;
use rustc_hash::FxHashSet;

use crate::reflect::helper::{SemanticErrorBlame, TextureData, UboData};
use crate::reflect::naga::spirv_passes::{link_input_outputs, lower_samplers};
use crate::reflect::semantics::{
    BindingMeta, BindingStage, BufferReflection, MemberOffset, ShaderSemantics, TextureBinding,
    TextureSemanticMap, TextureSemantics, TextureSizeMeta, TypeInfo, UniformMemberBlock,
    UniqueSemanticMap, UniqueSemantics, ValidateTypeSemantics, VariableMeta, MAX_BINDINGS_COUNT,
    MAX_PUSH_BUFFER_SIZE,
};
use crate::reflect::{align_uniform_size, ReflectShader, ShaderReflection};

/// Reflect under Naga semantics
///
/// The Naga reflector will lower combined image samplers to split,
/// with the same bind point on descriptor group 1.
pub struct Naga;

#[derive(Debug)]
pub(crate) struct NagaReflect {
    pub(crate) vertex: Module,
    pub(crate) fragment: Module,
}

/// Options to lower samplers and pcbs
#[derive(Debug, Default, Clone)]
pub struct NagaLoweringOptions {
    /// Whether to write the PCB as a UBO.
    pub write_pcb_as_ubo: bool,
    /// The bind group to assign samplers to. This is to ensure that samplers will
    /// maintain the same bindings as textures.
    pub sampler_bind_group: u32,
}

impl NagaReflect {
    pub fn do_lowering(&mut self, options: &NagaLoweringOptions) {
        if options.write_pcb_as_ubo {
            for (_, gv) in self.fragment.global_variables.iter_mut() {
                if gv.space == AddressSpace::PushConstant {
                    gv.space = AddressSpace::Uniform;
                }
            }

            for (_, gv) in self.vertex.global_variables.iter_mut() {
                if gv.space == AddressSpace::PushConstant {
                    gv.space = AddressSpace::Uniform;
                }
            }
        } else {
            for (_, gv) in self.fragment.global_variables.iter_mut() {
                if gv.space == AddressSpace::PushConstant {
                    gv.binding = None;
                }
            }
        }

        // Reassign shit.
        let images = self
            .fragment
            .global_variables
            .iter()
            .filter(|&(_, gv)| {
                let ty = &self.fragment.types[gv.ty];
                match ty.inner {
                    naga::TypeInner::Image { .. } => true,
                    naga::TypeInner::BindingArray { base, .. } => {
                        let ty = &self.fragment.types[base];
                        matches!(ty.inner, naga::TypeInner::Image { .. })
                    }
                    _ => false,
                }
            })
            .map(|(_, gv)| (gv.binding.clone(), gv.space))
            .collect::<naga::FastHashSet<_>>();

        self.fragment
            .global_variables
            .iter_mut()
            .filter(|(_, gv)| {
                let ty = &self.fragment.types[gv.ty];
                match ty.inner {
                    naga::TypeInner::Sampler { .. } => true,
                    naga::TypeInner::BindingArray { base, .. } => {
                        let ty = &self.fragment.types[base];
                        matches!(ty.inner, naga::TypeInner::Sampler { .. })
                    }
                    _ => false,
                }
            })
            .for_each(|(_, gv)| {
                if images.contains(&(gv.binding.clone(), gv.space)) {
                    if let Some(binding) = &mut gv.binding {
                        binding.group = options.sampler_bind_group;
                    }
                }
            });
    }
}

impl TryFrom<&SpirvCompilation> for NagaReflect {
    type Error = ShaderReflectError;

    fn try_from(compile: &SpirvCompilation) -> Result<Self, Self::Error> {
        fn load_module(words: &[u32]) -> rspirv::dr::Module {
            let mut loader = rspirv::dr::Loader::new();
            rspirv::binary::parse_words(words, &mut loader).unwrap();
            let module = loader.module();
            module
        }

        fn lower_fragment_shader(builder: &mut Builder) {
            let mut pass = lower_samplers::LowerCombinedImageSamplerPass::new(builder);
            pass.ensure_op_type_sampler();
            pass.do_pass();
        }

        let options = naga::front::spv::Options {
            adjust_coordinate_space: true,
            strict_capabilities: false,
            block_ctx_dump_prefix: None,
        };

        let vertex = load_module(&compile.vertex);
        let fragment = load_module(&compile.fragment);

        let mut fragment = Builder::new_from_module(fragment);
        lower_fragment_shader(&mut fragment);

        let mut pass = link_input_outputs::LinkInputs::new(&vertex, &mut fragment);
        pass.do_pass();

        let vertex = vertex.assemble();
        let fragment = fragment.module().assemble();

        let vertex = naga::front::spv::parse_u8_slice(bytemuck::cast_slice(&vertex), &options)?;

        let fragment = naga::front::spv::parse_u8_slice(bytemuck::cast_slice(&fragment), &options)?;

        Ok(NagaReflect { vertex, fragment })
    }
}

impl ValidateTypeSemantics<&TypeInner> for UniqueSemantics {
    fn validate_type(&self, ty: &&TypeInner) -> Option<TypeInfo> {
        let (TypeInner::Vector { .. } | TypeInner::Scalar { .. } | TypeInner::Matrix { .. }) = *ty
        else {
            return None;
        };

        match self {
            UniqueSemantics::MVP => {
                if matches!(ty, TypeInner::Matrix { columns, rows, scalar: Scalar { width, .. } } if *columns == VectorSize::Quad
                    && *rows == VectorSize::Quad && *width == 4)
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
                if matches!(ty, TypeInner::Scalar( Scalar { kind, width }) if *kind == ScalarKind::Uint && *width == 4)
                {
                    return Some(TypeInfo {
                        size: 1,
                        columns: 1,
                    });
                }
            }
            UniqueSemantics::FrameDirection => {
                // iint32 == width 4
                if matches!(ty, TypeInner::Scalar( Scalar { kind, width }) if *kind == ScalarKind::Sint && *width == 4)
                {
                    return Some(TypeInfo {
                        size: 1,
                        columns: 1,
                    });
                }
            }
            UniqueSemantics::FloatParameter => {
                // Float32 == width 4
                if matches!(ty, TypeInner::Scalar( Scalar { kind, width }) if *kind == ScalarKind::Float && *width == 4)
                {
                    return Some(TypeInfo {
                        size: 1,
                        columns: 1,
                    });
                }
            }
            _ => {
                if matches!(ty, TypeInner::Vector { scalar: Scalar { width, kind }, size } if *kind == ScalarKind::Float && *width == 4 && *size == VectorSize::Quad)
                {
                    return Some(TypeInfo {
                        size: 4,
                        columns: 1,
                    });
                }
            }
        };

        return None;
    }
}

impl ValidateTypeSemantics<&TypeInner> for TextureSemantics {
    fn validate_type(&self, ty: &&TypeInner) -> Option<TypeInfo> {
        let TypeInner::Vector {
            scalar: Scalar { width, kind },
            size,
        } = ty
        else {
            return None;
        };

        if *kind == ScalarKind::Float && *width == 4 && *size == VectorSize::Quad {
            return Some(TypeInfo {
                size: 4,
                columns: 1,
            });
        }

        None
    }
}

impl NagaReflect {
    fn reflect_ubos(
        &mut self,
        vertex_ubo: Option<Handle<GlobalVariable>>,
        fragment_ubo: Option<Handle<GlobalVariable>>,
    ) -> Result<Option<BufferReflection<u32>>, ShaderReflectError> {
        // todo: merge this with the spirv-cross code
        match (vertex_ubo, fragment_ubo) {
            (None, None) => Ok(None),
            (Some(vertex_ubo), Some(fragment_ubo)) => {
                let vertex_ubo = Self::get_ubo_data(
                    &self.vertex,
                    &self.vertex.global_variables[vertex_ubo],
                    SemanticErrorBlame::Vertex,
                )?;
                let fragment_ubo = Self::get_ubo_data(
                    &self.fragment,
                    &self.fragment.global_variables[fragment_ubo],
                    SemanticErrorBlame::Fragment,
                )?;
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
                let vertex_ubo = Self::get_ubo_data(
                    &self.vertex,
                    &self.vertex.global_variables[vertex_ubo],
                    SemanticErrorBlame::Vertex,
                )?;
                Ok(Some(BufferReflection {
                    binding: vertex_ubo.binding,
                    size: align_uniform_size(vertex_ubo.size),
                    stage_mask: BindingStage::VERTEX,
                }))
            }
            (None, Some(fragment_ubo)) => {
                let fragment_ubo = Self::get_ubo_data(
                    &self.fragment,
                    &self.fragment.global_variables[fragment_ubo],
                    SemanticErrorBlame::Fragment,
                )?;
                Ok(Some(BufferReflection {
                    binding: fragment_ubo.binding,
                    size: align_uniform_size(fragment_ubo.size),
                    stage_mask: BindingStage::FRAGMENT,
                }))
            }
        }
    }

    fn get_ubo_data(
        module: &Module,
        ubo: &GlobalVariable,
        blame: SemanticErrorBlame,
    ) -> Result<UboData, ShaderReflectError> {
        let Some(binding) = &ubo.binding else {
            return Err(blame.error(SemanticsErrorKind::MissingBinding));
        };

        if binding.binding >= MAX_BINDINGS_COUNT {
            return Err(blame.error(SemanticsErrorKind::InvalidBinding(binding.binding)));
        }

        if binding.group != 0 {
            return Err(blame.error(SemanticsErrorKind::InvalidDescriptorSet(binding.group)));
        }

        let ty = &module.types[ubo.ty];
        let size = ty.inner.size(module.to_ctx());
        Ok(UboData {
            // descriptor_set,
            // id: ubo.id,
            binding: binding.binding,
            size,
        })
    }

    fn get_next_binding(&self, bind_group: u32) -> u32 {
        let mut max_bind = 0;
        for (_, gv) in self.vertex.global_variables.iter() {
            let Some(binding) = &gv.binding else {
                continue;
            };
            if binding.group != bind_group {
                continue;
            }
            max_bind = std::cmp::max(max_bind, binding.binding);
        }

        for (_, gv) in self.fragment.global_variables.iter() {
            let Some(binding) = &gv.binding else {
                continue;
            };
            if binding.group != bind_group {
                continue;
            }
            max_bind = std::cmp::max(max_bind, binding.binding);
        }

        max_bind + 1
    }

    fn get_push_size(
        module: &Module,
        push: &GlobalVariable,
        blame: SemanticErrorBlame,
    ) -> Result<u32, ShaderReflectError> {
        let ty = &module.types[push.ty];
        let size = ty.inner.size(module.to_ctx());
        if size > MAX_PUSH_BUFFER_SIZE {
            return Err(blame.error(SemanticsErrorKind::InvalidPushBufferSize(size)));
        }
        Ok(size)
    }

    fn reflect_push_constant_buffer(
        &mut self,
        vertex_pcb: Option<Handle<GlobalVariable>>,
        fragment_pcb: Option<Handle<GlobalVariable>>,
    ) -> Result<Option<BufferReflection<Option<u32>>>, ShaderReflectError> {
        let binding = self.get_next_binding(0);
        // Reassign to UBO later if we want during compilation.
        if let Some(vertex_pcb) = vertex_pcb {
            let pcb = &mut self.vertex.global_variables[vertex_pcb];
            pcb.binding = Some(ResourceBinding { group: 0, binding });
        }

        if let Some(fragment_pcb) = fragment_pcb {
            let pcb = &mut self.fragment.global_variables[fragment_pcb];
            pcb.binding = Some(ResourceBinding { group: 0, binding });
        };

        match (vertex_pcb, fragment_pcb) {
            (None, None) => Ok(None),
            (Some(vertex_push), Some(fragment_push)) => {
                let vertex_size = Self::get_push_size(
                    &self.vertex,
                    &self.vertex.global_variables[vertex_push],
                    SemanticErrorBlame::Vertex,
                )?;
                let fragment_size = Self::get_push_size(
                    &self.fragment,
                    &self.fragment.global_variables[fragment_push],
                    SemanticErrorBlame::Fragment,
                )?;

                let size = std::cmp::max(vertex_size, fragment_size);

                Ok(Some(BufferReflection {
                    binding: Some(binding),
                    size: align_uniform_size(size),
                    stage_mask: BindingStage::VERTEX | BindingStage::FRAGMENT,
                }))
            }
            (Some(vertex_push), None) => {
                let vertex_size = Self::get_push_size(
                    &self.vertex,
                    &self.vertex.global_variables[vertex_push],
                    SemanticErrorBlame::Vertex,
                )?;
                Ok(Some(BufferReflection {
                    binding: Some(binding),
                    size: align_uniform_size(vertex_size),
                    stage_mask: BindingStage::VERTEX,
                }))
            }
            (None, Some(fragment_push)) => {
                let fragment_size = Self::get_push_size(
                    &self.fragment,
                    &self.fragment.global_variables[fragment_push],
                    SemanticErrorBlame::Fragment,
                )?;
                Ok(Some(BufferReflection {
                    binding: Some(binding),
                    size: align_uniform_size(fragment_size),
                    stage_mask: BindingStage::FRAGMENT,
                }))
            }
        }
    }

    fn validate(&self) -> Result<(), ShaderReflectError> {
        // Verify types
        if self.vertex.global_variables.iter().any(|(_, gv)| {
            let ty = &self.vertex.types[gv.ty];
            match ty.inner {
                TypeInner::Scalar { .. }
                | TypeInner::Vector { .. }
                | TypeInner::Matrix { .. }
                | TypeInner::Struct { .. } => false,
                _ => true,
            }
        }) {
            return Err(ShaderReflectError::VertexSemanticError(
                SemanticsErrorKind::InvalidResourceType,
            ));
        }

        if self.fragment.global_variables.iter().any(|(_, gv)| {
            let ty = &self.fragment.types[gv.ty];
            match ty.inner {
                TypeInner::Scalar { .. }
                | TypeInner::Vector { .. }
                | TypeInner::Matrix { .. }
                | TypeInner::Struct { .. }
                | TypeInner::Image { .. }
                | TypeInner::Sampler { .. } => false,
                TypeInner::BindingArray { base, .. } => {
                    let ty = &self.fragment.types[base];
                    match ty.inner {
                        TypeInner::Image { class, .. }
                            if !matches!(class, ImageClass::Storage { .. }) =>
                        {
                            false
                        }
                        TypeInner::Sampler { .. } => false,
                        _ => true,
                    }
                }
                _ => true,
            }
        }) {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidResourceType,
            ));
        }

        // Verify Vertex inputs
        'vertex: {
            if self.vertex.entry_points.len() != 1 {
                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::InvalidEntryPointCount(self.vertex.entry_points.len()),
                ));
            }

            let vertex_entry_point = &self.vertex.entry_points[0];
            let vert_inputs = vertex_entry_point.function.arguments.len();
            if vert_inputs != 2 {
                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::InvalidInputCount(vert_inputs),
                ));
            }
            for input in &vertex_entry_point.function.arguments {
                let &Some(Binding::Location { location, .. }) = &input.binding else {
                    return Err(ShaderReflectError::VertexSemanticError(
                        SemanticsErrorKind::MissingBinding,
                    ));
                };

                if location == 0 {
                    let pos_type = &self.vertex.types[input.ty];
                    if !matches!(pos_type.inner, TypeInner::Vector { size, ..} if size == VectorSize::Quad)
                    {
                        return Err(ShaderReflectError::VertexSemanticError(
                            SemanticsErrorKind::InvalidLocation(location),
                        ));
                    }
                    break 'vertex;
                }

                if location == 1 {
                    let coord_type = &self.vertex.types[input.ty];
                    if !matches!(coord_type.inner, TypeInner::Vector { size, ..} if size == VectorSize::Bi)
                    {
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

            let uniform_buffer_count = self
                .vertex
                .global_variables
                .iter()
                .filter(|(_, gv)| gv.space == AddressSpace::Uniform)
                .count();

            if uniform_buffer_count > 1 {
                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::InvalidUniformBufferCount(uniform_buffer_count),
                ));
            }

            let push_buffer_count = self
                .vertex
                .global_variables
                .iter()
                .filter(|(_, gv)| gv.space == AddressSpace::PushConstant)
                .count();

            if push_buffer_count > 1 {
                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::InvalidPushBufferCount(push_buffer_count),
                ));
            }
        }

        {
            if self.fragment.entry_points.len() != 1 {
                return Err(ShaderReflectError::FragmentSemanticError(
                    SemanticsErrorKind::InvalidEntryPointCount(self.vertex.entry_points.len()),
                ));
            }

            let frag_entry_point = &self.fragment.entry_points[0];
            let Some(frag_output) = &frag_entry_point.function.result else {
                return Err(ShaderReflectError::FragmentSemanticError(
                    SemanticsErrorKind::InvalidOutputCount(0),
                ));
            };

            let &Some(Binding::Location { location, .. }) = &frag_output.binding else {
                return Err(ShaderReflectError::VertexSemanticError(
                    SemanticsErrorKind::MissingBinding,
                ));
            };

            if location != 0 {
                return Err(ShaderReflectError::FragmentSemanticError(
                    SemanticsErrorKind::InvalidLocation(location),
                ));
            }

            let uniform_buffer_count = self
                .fragment
                .global_variables
                .iter()
                .filter(|(_, gv)| gv.space == AddressSpace::Uniform)
                .count();

            if uniform_buffer_count > 1 {
                return Err(ShaderReflectError::FragmentSemanticError(
                    SemanticsErrorKind::InvalidUniformBufferCount(uniform_buffer_count),
                ));
            }

            let push_buffer_count = self
                .fragment
                .global_variables
                .iter()
                .filter(|(_, gv)| gv.space == AddressSpace::PushConstant)
                .count();

            if push_buffer_count > 1 {
                return Err(ShaderReflectError::FragmentSemanticError(
                    SemanticsErrorKind::InvalidPushBufferCount(push_buffer_count),
                ));
            }
        }

        Ok(())
    }

    fn collect_uniform_names(
        module: &Module,
        buffer_handle: Handle<GlobalVariable>,
        blame: SemanticErrorBlame,
    ) -> Result<FxHashSet<&StructMember>, ShaderReflectError> {
        let mut names = FxHashSet::default();
        let ubo = &module.global_variables[buffer_handle];

        let TypeInner::Struct { members, .. } = &module.types[ubo.ty].inner else {
            return Err(blame.error(SemanticsErrorKind::InvalidResourceType));
        };

        // struct access is AccessIndex
        for (_, fun) in module.functions.iter() {
            for (_, expr) in fun.expressions.iter() {
                let &Expression::AccessIndex { base, index } = expr else {
                    continue;
                };

                let &Expression::GlobalVariable(base) = &fun.expressions[base] else {
                    continue;
                };

                if base == buffer_handle {
                    let member = members
                        .get(index as usize)
                        .ok_or(blame.error(SemanticsErrorKind::InvalidRange(index)))?;
                    names.insert(member);
                }
            }
        }

        Ok(names)
    }

    fn reflect_buffer_struct_members(
        module: &Module,
        resource: Handle<GlobalVariable>,
        pass_number: usize,
        semantics: &ShaderSemantics,
        meta: &mut BindingMeta,
        offset_type: UniformMemberBlock,
        blame: SemanticErrorBlame,
    ) -> Result<(), ShaderReflectError> {
        let reachable = Self::collect_uniform_names(&module, resource, blame)?;

        let resource = &module.global_variables[resource];

        let TypeInner::Struct { members, .. } = &module.types[resource.ty].inner else {
            return Err(blame.error(SemanticsErrorKind::InvalidResourceType));
        };

        for member in members {
            let Some(name) = member.name.clone() else {
                return Err(blame.error(SemanticsErrorKind::InvalidRange(member.offset)));
            };

            if !reachable.contains(member) {
                continue;
            }

            let member_type = &module.types[member.ty].inner;

            if let Some(parameter) = semantics.uniform_semantics.get_unique_semantic(&name) {
                let Some(typeinfo) = parameter.semantics.validate_type(&member_type) else {
                    return Err(blame.error(SemanticsErrorKind::InvalidTypeForSemantic(name)));
                };

                match &parameter.semantics {
                    UniqueSemantics::FloatParameter => {
                        let offset = member.offset;
                        if let Some(meta) = meta.parameter_meta.get_mut(&name) {
                            if let Some(expected) = meta.offset.offset(offset_type)
                                && expected != offset as usize
                            {
                                return Err(ShaderReflectError::MismatchedOffset {
                                    semantic: name,
                                    expected,
                                    received: offset as usize,
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

                            *meta.offset.offset_mut(offset_type) = Some(offset as usize);
                        } else {
                            meta.parameter_meta.insert(
                                name.clone(),
                                VariableMeta {
                                    id: name,
                                    offset: MemberOffset::new(offset as usize, offset_type),
                                    size: typeinfo.size,
                                },
                            );
                        }
                    }
                    semantics => {
                        let offset = member.offset;
                        if let Some(meta) = meta.unique_meta.get_mut(semantics) {
                            if let Some(expected) = meta.offset.offset(offset_type)
                                && expected != offset as usize
                            {
                                return Err(ShaderReflectError::MismatchedOffset {
                                    semantic: name,
                                    expected,
                                    received: offset as usize,
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

                            *meta.offset.offset_mut(offset_type) = Some(offset as usize);
                        } else {
                            meta.unique_meta.insert(
                                *semantics,
                                VariableMeta {
                                    id: name,
                                    offset: MemberOffset::new(offset as usize, offset_type),
                                    size: typeinfo.size * typeinfo.columns,
                                },
                            );
                        }
                    }
                }
            } else if let Some(texture) = semantics.uniform_semantics.get_texture_semantic(&name) {
                let Some(_typeinfo) = texture.semantics.validate_type(&member_type) else {
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

                let offset = member.offset;
                if let Some(meta) = meta.texture_size_meta.get_mut(&texture) {
                    if let Some(expected) = meta.offset.offset(offset_type)
                        && expected != offset as usize
                    {
                        return Err(ShaderReflectError::MismatchedOffset {
                            semantic: name,
                            expected,
                            received: offset as usize,
                            ty: offset_type,
                            pass: pass_number,
                        });
                    }

                    meta.stage_mask.insert(match blame {
                        SemanticErrorBlame::Vertex => BindingStage::VERTEX,
                        SemanticErrorBlame::Fragment => BindingStage::FRAGMENT,
                    });

                    *meta.offset.offset_mut(offset_type) = Some(offset as usize);
                } else {
                    meta.texture_size_meta.insert(
                        texture,
                        TextureSizeMeta {
                            offset: MemberOffset::new(offset as usize, offset_type),
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

    fn reflect_texture<'a>(
        &'a self,
        texture: &'a GlobalVariable,
    ) -> Result<TextureData<'a>, ShaderReflectError> {
        let Some(binding) = &texture.binding else {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::MissingBinding,
            ));
        };

        let Some(name) = texture.name.as_ref() else {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidBinding(binding.binding),
            ));
        };

        if binding.group != 0 {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidDescriptorSet(binding.group),
            ));
        }
        if binding.binding >= MAX_BINDINGS_COUNT {
            return Err(ShaderReflectError::FragmentSemanticError(
                SemanticsErrorKind::InvalidBinding(binding.binding),
            ));
        }

        Ok(TextureData {
            // id: texture.id,
            // descriptor_set,
            name: &name,
            binding: binding.binding,
        })
    }

    // todo: share this with cross
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
}

impl ReflectShader for NagaReflect {
    fn reflect(
        &mut self,
        pass_number: usize,
        semantics: &ShaderSemantics,
    ) -> Result<ShaderReflection, ShaderReflectError> {
        self.validate()?;

        // Validate verifies that there's only one uniform block.
        let vertex_ubo = self
            .vertex
            .global_variables
            .iter()
            .find_map(|(handle, gv)| {
                if gv.space == AddressSpace::Uniform {
                    Some(handle)
                } else {
                    None
                }
            });

        let fragment_ubo = self
            .fragment
            .global_variables
            .iter()
            .find_map(|(handle, gv)| {
                if gv.space == AddressSpace::Uniform {
                    Some(handle)
                } else {
                    None
                }
            });

        let ubo = self.reflect_ubos(vertex_ubo, fragment_ubo)?;

        let vertex_push = self
            .vertex
            .global_variables
            .iter()
            .find_map(|(handle, gv)| {
                if gv.space == AddressSpace::PushConstant {
                    Some(handle)
                } else {
                    None
                }
            });

        let fragment_push = self
            .fragment
            .global_variables
            .iter()
            .find_map(|(handle, gv)| {
                if gv.space == AddressSpace::PushConstant {
                    Some(handle)
                } else {
                    None
                }
            });

        let push_constant = self.reflect_push_constant_buffer(vertex_push, fragment_push)?;
        let mut meta = BindingMeta::default();

        if let Some(ubo) = vertex_ubo {
            Self::reflect_buffer_struct_members(
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
            Self::reflect_buffer_struct_members(
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
            Self::reflect_buffer_struct_members(
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
            Self::reflect_buffer_struct_members(
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

        let textures = self.fragment.global_variables.iter().filter(|(_, gv)| {
            let ty = &self.fragment.types[gv.ty];
            matches!(ty.inner, TypeInner::Image { .. })
        });

        for (_, texture) in textures {
            let texture_data = self.reflect_texture(texture)?;
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
    use crate::reflect::semantics::{Semantic, TextureSemantics, UniformSemantic};
    use librashader_common::map::FastHashMap;
    use librashader_preprocess::ShaderSource;
    use librashader_presets::ShaderPreset;

    // #[test]
    // pub fn test_into() {
    //     let result = ShaderSource::load("../test/slang-shaders/crt/shaders/crt-royale/src/crt-royale-scanlines-horizontal-apply-mask.slang").unwrap();
    //     let compilation = crate::front::GlslangCompilation::try_from(&result).unwrap();
    //
    //     let mut loader = rspirv::dr::Loader::new();
    //     rspirv::binary::parse_words(compilation.vertex.as_binary(), &mut loader).unwrap();
    //     let module = loader.module();
    //
    //     let outputs: Vec<&Instruction> = module
    //         .types_global_values
    //         .iter()
    //         .filter(|i| i.class.opcode == Op::Variable)
    //         .collect();
    //
    //     println!("{outputs:#?}");
    // }

    // #[test]
    // pub fn mega_bezel_reflect() {
    //     let preset = ShaderPreset::try_parse(
    //         "../test/shaders_slang/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp",
    //     )
    //         .unwrap();
    //
    //     let mut uniform_semantics: FastHashMap<String, UniformSemantic> = Default::default();
    //     let mut texture_semantics: FastHashMap<String, Semantic<TextureSemantics>> = Default::default();
    //
    //
    //
    //
    // }
}
