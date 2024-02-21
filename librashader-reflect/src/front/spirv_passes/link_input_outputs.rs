use rspirv::dr::{Builder, Module, Operand};
use rustc_hash::{FxHashMap, FxHashSet};
use spirv::{Decoration, Op, StorageClass};

pub struct LinkInputs<'a> {
    pub frag_builder: &'a mut Builder,
    pub inputs: FxHashSet<spirv::Word>,
}

impl<'a> LinkInputs<'a> {
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

    pub fn new(vert: &'a Module, frag: &'a mut Builder) -> Self {
        let mut inputs = FxHashSet::default();
        let mut bindings = FxHashMap::default();
        for global in frag.module_ref().types_global_values.iter() {
            if global.class.opcode == spirv::Op::Variable
                && global.operands[0] == Operand::StorageClass(StorageClass::Input)
            {
                if let Some(id) = global.result_id {
                    let Some(location) = Self::find_location(frag.module_ref(), id) else {
                        continue;
                    };

                    inputs.insert(id);
                    bindings.insert(location, id);
                }
            }
        }

        for global in vert.types_global_values.iter() {
            if global.class.opcode == spirv::Op::Variable
                && global.operands[0] == Operand::StorageClass(StorageClass::Output)
            {
                if let Some(id) = global.result_id {
                    let Some(location) = Self::find_location(vert, id) else {
                        continue;
                    };
                    if let Some(frag_ref) = bindings.get(&location) {
                        // if something is bound to the same location in the vertex shader,
                        // we're good.
                        inputs.remove(&frag_ref);
                    }
                }
            }
        }

        Self {
            frag_builder: frag,
            inputs,
        }
    }

    pub fn do_pass(&mut self) {
        let functions = &self.frag_builder.module_ref().functions;

        // literally if it has any reference at all we can keep it
        for function in functions {
            for param in &function.parameters {
                for op in &param.operands {
                    if let Some(word) = op.id_ref_any() {
                        if self.inputs.contains(&word) {
                            self.inputs.remove(&word);
                        }
                    }
                }
            }

            for block in &function.blocks {
                for inst in &block.instructions {
                    for op in &inst.operands {
                        if let Some(word) = op.id_ref_any() {
                            if self.inputs.contains(&word) {
                                self.inputs.remove(&word);
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
                    if self.inputs.contains(&word) {
                        return false;
                    }
                }
            }
            return true;
        });

        self.frag_builder.module_mut().annotations.retain(|instr| {
            for op in &instr.operands {
                if let Some(word) = op.id_ref_any() {
                    if self.inputs.contains(&word) {
                        return false;
                    }
                }
            }
            return true;
        });

        for entry_point in self.frag_builder.module_mut().entry_points.iter_mut() {
            entry_point.operands.retain(|op| {
                if let Some(word) = op.id_ref_any() {
                    if self.inputs.contains(&word) {
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

                !self.inputs.contains(&id)
            });
    }
}
