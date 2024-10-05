use rspirv::dr::{Builder, Module, Operand};
use rustc_hash::{FxHashMap, FxHashSet};
use spirv::{Decoration, Op, StorageClass};

/// Do DCE on inputs of the fragment shader, then
/// link by downgrading outputs of unused fragment inputs
/// to global variables on the vertex shader.
pub struct LinkInputs<'a> {
    pub frag_builder: &'a mut Builder,
    pub vert_builder: &'a mut Builder,

    // binding -> ID
    pub outputs: FxHashMap<u32, spirv::Word>,
    // id -> binding
    pub inputs_to_remove: FxHashMap<spirv::Word, u32>,
}

impl<'a> LinkInputs<'a> {
    /// Get the value of the location of the inout in the module
    fn find_location(module: &Module, id: spirv::Word) -> Option<u32> {
        module.annotations.iter().find_map(|op| {
            if op.class.opcode != Op::Decorate {
                return None;
            }

            let Some(Operand::Decoration(Decoration::Location)) = op.operands.get(1) else {
                return None;
            };

            let Some(&Operand::IdRef(target)) = op.operands.get(0) else {
                return None;
            };

            if target != id {
                return None;
            }

            let Some(&Operand::LiteralBit32(binding)) = op.operands.get(2) else {
                return None;
            };
            return Some(binding);
        })
    }

    pub fn new(vert: &'a mut Builder, frag: &'a mut Builder, keep_if_bound: bool) -> Self {
        let mut outputs = FxHashMap::default();
        let mut inputs_to_remove = FxHashMap::default();
        let mut inputs = FxHashMap::default();

        for global in frag.module_ref().types_global_values.iter() {
            if global.class.opcode == spirv::Op::Variable
                && global.operands[0] == Operand::StorageClass(StorageClass::Input)
            {
                if let Some(id) = global.result_id {
                    let Some(location) = Self::find_location(frag.module_ref(), id) else {
                        continue;
                    };

                    inputs_to_remove.insert(id, location);
                    inputs.insert(location, id);
                }
            }
        }

        for global in vert.module_ref().types_global_values.iter() {
            if global.class.opcode == spirv::Op::Variable
                && global.operands[0] == Operand::StorageClass(StorageClass::Output)
            {
                if let Some(id) = global.result_id {
                    let Some(location) = Self::find_location(vert.module_ref(), id) else {
                        continue;
                    };

                    // Add to list of outputs
                    outputs.insert(location, id);

                    // Keep the input, if it is bound to both stages.Otherwise, do DCE analysis on
                    // the input, and remove it regardless if bound, if unused by the fragment stage.
                    if keep_if_bound {
                        if let Some(frag_ref) = inputs.get(&location) {
                            // if something is bound to the same location in the vertex shader,
                            // we're good.
                            inputs_to_remove.remove(&frag_ref);
                        }
                    }
                }
            }
        }

        Self {
            frag_builder: frag,
            vert_builder: vert,
            outputs,
            inputs_to_remove,
        }
    }

    pub fn do_pass(&mut self) {
        self.trim_inputs();
        self.downgrade_outputs();
    }

    /// Downgrade dead inputs corresponding to outputs to global variables, keeping existing mappings.
    fn downgrade_outputs(&mut self) {
        let dead_outputs = self
            .inputs_to_remove
            .values()
            .filter_map(|i| self.outputs.get(i).cloned())
            .collect::<FxHashSet<spirv::Word>>();

        let mut pointer_types_to_downgrade = FxHashSet::default();

        // Map from Pointer type to pointee
        let mut pointer_type_pointee = Vec::new();

        // Map from StorageClass Output to StorageClass Private
        let mut downgraded_pointer_types = FxHashMap::default();

        // First collect all the pointer types that are needed for dead outputs.
        for global in self.vert_builder.module_ref().types_global_values.iter() {
            if global.class.opcode != spirv::Op::Variable
                || global.operands[0] != Operand::StorageClass(StorageClass::Output)
            {
                continue;
            }

            if let Some(id) = global.result_id {
                if !dead_outputs.contains(&id) {
                    continue;
                }

                if let Some(result_type) = global.result_type {
                    pointer_types_to_downgrade.insert(result_type);
                }
            }
        }

        // Collect all the pointee types of pointer types to downgrade
        for global in self.vert_builder.module_ref().types_global_values.iter() {
            if global.class.opcode != spirv::Op::TypePointer
                || global.operands[0] != Operand::StorageClass(StorageClass::Output)
            {
                continue;
            }

            if let Some(id) = global.result_id {
                if !pointer_types_to_downgrade.contains(&id) {
                    continue;
                }

                let Some(pointee_type) = global.operands[1].id_ref_any() else {
                    continue;
                };

                pointer_type_pointee.push((id, pointee_type));
            }
        }

        // Create pointer types for everything we saw above with Private storage class.
        // We don't have to deal with OpTypeForwardPointer, because PhysicalStorageBuffer
        // is not valid in slang shaders, and we're only working with Vulkan inputs.
        for (pointer_type, pointee_type) in pointer_type_pointee.into_iter() {
            // Create a new private type
            let private_pointer_type =
                self.vert_builder
                    .type_pointer(None, StorageClass::Private, pointee_type);

            // Add it to the mapping
            downgraded_pointer_types.insert(pointer_type, private_pointer_type);
        }

        // Downgrade the OpVariable storage class and reassign the types.
        for global in self
            .vert_builder
            .module_mut()
            .types_global_values
            .iter_mut()
        {
            if global.class.opcode != spirv::Op::Variable
                || global.operands[0] != Operand::StorageClass(StorageClass::Output)
            {
                continue;
            }

            if let Some(id) = global.result_id {
                if !dead_outputs.contains(&id) {
                    continue;
                }

                // downgrade the OpVariable if it's in dead_outputs
                global.operands[0] = Operand::StorageClass(StorageClass::Private);

                // Get the result type. If there's no result type it's invalid anyways
                // so it doesn't matter that we downgraded early (better downgraded than unmatched)
                let Some(result_type) = &mut global.result_type else {
                    continue;
                };

                let Some(new_type) = downgraded_pointer_types.get(&result_type) else {
                    // We should have created one above.
                    continue;
                };

                // Set the type of the OpVariable to the same type with Private storageclass.
                *result_type = *new_type;
            }
        }

        // Strip decorations of downgraded variables.
        self.vert_builder.module_mut().annotations.retain_mut(|op| {
            if op.class.opcode != Op::Decorate {
                return true;
            }

            let Some(Operand::Decoration(Decoration::Location)) = op.operands.get(1) else {
                return true;
            };

            let Some(&Operand::IdRef(target)) = op.operands.get(0) else {
                return true;
            };

            // If target is in dead outputs, then don't keep it.
            !dead_outputs.contains(&target)
        });

        for entry_point in self.vert_builder.module_mut().entry_points.iter_mut() {
            let mut index = 0;
            entry_point.operands.retain(|s| {
                // Skip the execution mode, entry point reference, and name.
                if index < 3 {
                    index += 1;
                    return true;
                }

                index += 1;

                // Ignore any non-IdRef
                let Operand::IdRef(id_ref) = s else {
                    return true;
                };

                // If the entry point contains a dead outputs, remove it from the interface.
                !dead_outputs.contains(id_ref)
            });
        }
    }

    // Trim unused fragment shader inputs
    fn trim_inputs(&mut self) {
        let functions = &self.frag_builder.module_ref().functions;

        // literally if it has any reference at all we can keep it
        for function in functions {
            for param in &function.parameters {
                for op in &param.operands {
                    if let Some(word) = op.id_ref_any() {
                        if self.inputs_to_remove.contains_key(&word) {
                            self.inputs_to_remove.remove(&word);
                        }
                    }
                }
            }

            for block in &function.blocks {
                for inst in &block.instructions {
                    for op in &inst.operands {
                        if let Some(word) = op.id_ref_any() {
                            if self.inputs_to_remove.contains_key(&word) {
                                self.inputs_to_remove.remove(&word);
                            }
                        }
                    }
                }
            }
        }

        // ok well guess we dont
        self.frag_builder.module_mut().debug_names.retain(|instr| {
            for op in &instr.operands {
                if let Some(word) = op.id_ref_any() {
                    if self.inputs_to_remove.contains_key(&word) {
                        return false;
                    }
                }
            }
            return true;
        });

        self.frag_builder.module_mut().annotations.retain(|instr| {
            for op in &instr.operands {
                if let Some(word) = op.id_ref_any() {
                    if self.inputs_to_remove.contains_key(&word) {
                        return false;
                    }
                }
            }
            return true;
        });

        for entry_point in self.frag_builder.module_mut().entry_points.iter_mut() {
            entry_point.operands.retain(|op| {
                if let Some(word) = op.id_ref_any() {
                    if self.inputs_to_remove.contains_key(&word) {
                        return false;
                    }
                }
                return true;
            })
        }

        self.frag_builder
            .module_mut()
            .types_global_values
            .retain(|instr| {
                let Some(id) = instr.result_id else {
                    return true;
                };

                !self.inputs_to_remove.contains_key(&id)
            });
    }
}
