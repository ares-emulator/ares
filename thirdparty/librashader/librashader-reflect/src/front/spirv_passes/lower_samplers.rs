use rspirv::dr::{Builder, Instruction, Operand};
use rustc_hash::{FxHashMap, FxHashSet};
use spirv::{Op, StorageClass, Word};
use std::borrow::Cow;
use std::collections::hash_map::Entry;

pub struct LowerCombinedImageSamplerPass<'a> {
    pub builder: &'a mut Builder,
    pub seen_functions: FxHashSet<spirv::Word>,
}

#[derive(Clone, Debug)]
struct CombinedImageSampler {
    sampler_variable: spirv::Word,
    sampler_pointer_type: spirv::Word,
    original_uniform_pointer_type_id: spirv::Word,
    original_uniform_type: Instruction,

    target_texture_type_id: spirv::Word,
    target_texture_pointer_type_id: spirv::Word,

    // Always OpTypeImage
    base_type: Instruction,
}

#[derive(Debug)]
struct OpAccessChain<'a> {
    sampled_image: &'a CombinedImageSampler,
    index: Operand,
    original_result_type: Word,
    target_pointer_type: Word,
}

impl<'a> LowerCombinedImageSamplerPass<'a> {
    pub fn new(builder: &'a mut Builder) -> Self {
        let val = Self {
            builder,
            seen_functions: FxHashSet::default(),
        };

        val
    }

    pub(crate) fn do_pass(&mut self) {
        let mut combined_image_samplers = self.collect_global_sampled_images();
        self.retype_combined_image_sampler_uniforms(&combined_image_samplers);
        self.put_variables_to_end();

        // First rewrite global loads
        self.rewrite_loads(&combined_image_samplers);

        let mut op_access_chains = self.collect_op_access_chain(&combined_image_samplers);
        self.rewrite_global_op_access_chain_loads(&op_access_chains);

        // Collect functions that reference combined image samplers indirectly (todo) (also rewrite the found OpFunctionCall)

        while !combined_image_samplers.is_empty() {
            let op_functions =
                self.rewrite_function_calls(&op_access_chains, &combined_image_samplers);

            combined_image_samplers = self.rewrite_functions_definitions(&op_functions);
            self.rewrite_loads(&combined_image_samplers);

            op_access_chains = self.collect_op_access_chain(&combined_image_samplers);

            self.rewrite_global_op_access_chain_loads(&op_access_chains);
        }

        // notes: OpFunctionCall %void %assign_s21_ %tex (i.e for scalar no OpLoad precedes, OpLoad and OpAccessChain is in function), this is true even when passing array of samplers
        //        However, if passing single sampler from array, OpAccessChain happens in outer scope.
        // Rewrite functions that do so by adding sampler param
        // rewrite loads + op_access_chains for functions
    }

    fn put_variables_to_end(&mut self) {
        // this is easier than doing proper topo sort.
        let mut vars = Vec::new();

        self.builder
            .module_mut()
            .types_global_values
            .retain(|instr| {
                if instr.class.opcode == spirv::Op::Variable {
                    vars.push(instr.clone());
                    return false;
                };
                true
            });

        self.builder
            .module_mut()
            .types_global_values
            .append(&mut vars);
    }
    pub(crate) fn ensure_op_type_sampler(&mut self) {
        self.builder.type_sampler();
    }

    fn find_global_instruction(&self, word: Word) -> Option<&Instruction> {
        self.builder
            .module_ref()
            .global_inst_iter()
            .find(|i| i.result_id == Some(word))
    }

    fn create_sampler_name(&mut self, word: Word) -> Option<String> {
        self.builder.module_ref().debug_names.iter().find_map(|i| {
            if i.class.opcode != spirv::Op::Name {
                return None;
            }

            let Some(&Operand::IdRef(target)) = &i.operands.get(0) else {
                return None;
            };

            if target != word {
                return None;
            }

            let Some(Operand::LiteralString(string)) = &i.operands.get(1) else {
                return None;
            };

            return Some(format!("_{string}_sampler"));
        })
    }

    fn get_base_type_for_sampled_image(&'a self, inst: &'a Instruction) -> Option<&'a Instruction> {
        if inst.class.opcode != spirv::Op::TypeSampledImage {
            return None;
        }

        let Some(&Operand::IdRef(referand)) = inst.operands.get(0) else {
            return None;
        };

        self.find_global_instruction(referand)
    }

    // Create a sampler OpVariable at the selected block
    fn create_sampler_uniform(
        &mut self,
        uniform_type: spirv::Word,
        combined_image_sampler: spirv::Word,
    ) -> (spirv::Word, spirv::Word) {
        let sampler_pointer_type =
            self.builder
                .type_pointer(None, StorageClass::UniformConstant, uniform_type);

        let sampler_uniform = self.builder.variable(
            sampler_pointer_type,
            None,
            StorageClass::UniformConstant,
            None,
        );

        let decorations: Vec<Instruction> = self
            .builder
            .module_ref()
            .annotations
            .iter()
            .filter_map(|f| {
                if f.class.opcode == spirv::Op::Decorate
                    && f.operands[0] == Operand::IdRef(combined_image_sampler)
                {
                    Some(f.clone())
                } else {
                    None
                }
            })
            .collect();

        if let Some(name) = self.create_sampler_name(combined_image_sampler) {
            self.builder.name(sampler_uniform, name);
        }

        // Clone decorations to the created sampler
        for decoration in decorations {
            let Operand::Decoration(decoration_type) = decoration.operands[1] else {
                continue;
            };

            self.builder.decorate(
                sampler_uniform,
                decoration_type,
                decoration.operands[2..].iter().map(|f| f.clone()),
            )
        }

        (sampler_pointer_type, sampler_uniform)
    }
    fn collect_global_sampled_images(&mut self) -> FxHashMap<spirv::Word, CombinedImageSampler> {
        let mut image_sampler_cadidates = Vec::new();
        let mut image_sampler_types = FxHashMap::default();

        for global in self.builder.module_ref().types_global_values.iter() {
            if global.class.opcode == spirv::Op::Variable
                && global.operands[0] == Operand::StorageClass(StorageClass::UniformConstant)
            {
                let pointer_type = global.result_type;
                let Some(pointer_type) = pointer_type else {
                    continue;
                };
                image_sampler_cadidates.push((pointer_type, global.result_id))
            }
        }

        for (pointer_type_id, global_variable) in image_sampler_cadidates {
            let Some(pointer_type) = self.find_global_instruction(pointer_type_id).cloned() else {
                continue;
            };

            if pointer_type.class.opcode == spirv::Op::TypePointer
                && pointer_type.operands[0] == Operand::StorageClass(StorageClass::UniformConstant)
            {
                let Some(&Operand::IdRef(sampled_image_type)) = pointer_type.operands.get(1) else {
                    continue;
                };

                let Some(uniform_type) = self.find_global_instruction(sampled_image_type).cloned()
                else {
                    continue;
                };

                if uniform_type.class.opcode == spirv::Op::TypeSampledImage {
                    let Some(base_type) =
                        self.get_base_type_for_sampled_image(&uniform_type).cloned()
                    else {
                        continue;
                    };

                    let Some(combined_image_sampler) = global_variable else {
                        continue;
                    };

                    // insert the sampler
                    if base_type.class.opcode != spirv::Op::TypeImage {
                        continue;
                    }

                    let Some(base_type_id) = base_type.result_id else {
                        continue;
                    };

                    let sampler_type = self.builder.type_sampler();

                    let (sampler_pointer_type, sampler_uniform) =
                        self.create_sampler_uniform(sampler_type, combined_image_sampler);

                    image_sampler_types.insert(
                        combined_image_sampler,
                        CombinedImageSampler {
                            sampler_variable: sampler_uniform,
                            original_uniform_type: uniform_type,
                            target_texture_type_id: base_type_id,
                            original_uniform_pointer_type_id: pointer_type_id,
                            base_type,
                            sampler_pointer_type,
                            target_texture_pointer_type_id: pointer_type_id,
                        },
                    );

                    continue;
                }

                if uniform_type.class.opcode == spirv::Op::TypeArray {
                    let Some(&Operand::IdRef(array_base_type)) = uniform_type.operands.get(0)
                    else {
                        continue;
                    };

                    let Some(&Operand::IdRef(array_length)) = uniform_type.operands.get(1) else {
                        continue;
                    };

                    let Some(sampled_image_type) =
                        self.find_global_instruction(array_base_type).cloned()
                    else {
                        continue;
                    };

                    let Some(base_type) = self
                        .get_base_type_for_sampled_image(&sampled_image_type)
                        .cloned()
                    else {
                        continue;
                    };

                    let Some(combined_image_sampler) = global_variable else {
                        continue;
                    };

                    // insert the sampler
                    if base_type.class.opcode != spirv::Op::TypeImage {
                        continue;
                    }

                    let sampler_type = self.builder.type_sampler();
                    let sampler_array_type = self.builder.type_array(sampler_type, array_length);

                    let (sampler_pointer_type, sampler_uniform) =
                        self.create_sampler_uniform(sampler_array_type, combined_image_sampler);

                    // insert target types
                    let Some(base_type_id) = base_type.result_id else {
                        continue;
                    };

                    let target_texture_type_id =
                        self.builder.type_array(base_type_id, array_length);

                    let target_texture_pointer_type_id = self.builder.type_pointer(
                        None,
                        StorageClass::UniformConstant,
                        target_texture_type_id,
                    );

                    image_sampler_types.insert(
                        combined_image_sampler,
                        CombinedImageSampler {
                            sampler_variable: sampler_uniform,
                            original_uniform_type: uniform_type,
                            target_texture_type_id,
                            original_uniform_pointer_type_id: pointer_type_id,
                            base_type,
                            sampler_pointer_type,
                            target_texture_pointer_type_id,
                        },
                    );
                    continue;
                }
            }
        }

        image_sampler_types
    }

    fn retype_combined_image_sampler_uniforms(
        &mut self,
        combined_image_samplers: &FxHashMap<spirv::Word, CombinedImageSampler>,
    ) {
        // Need to rebuild the global instructions because we need to insert new types...
        let mut instructions = Vec::new();
        for instr in self.builder.module_ref().types_global_values.clone() {
            let Some(result_id) = instr.result_id else {
                instructions.push(instr);
                continue;
            };

            let Some(sampled_image) = combined_image_samplers.get(&result_id) else {
                // We need to fix..
                instructions.push(instr);
                continue;
            };

            let Some(_base_type_id) = sampled_image.base_type.result_id else {
                instructions.push(instr);
                continue;
            };

            // If it's a OpTypeSampledImage, we want to change the variable type to &TypeImage.
            if sampled_image.original_uniform_type.class.opcode == spirv::Op::TypeSampledImage {
                // keep labels in sync
                let mut op_variable = instr;
                op_variable.result_type = Some(sampled_image.target_texture_pointer_type_id);

                instructions.push(op_variable);
                continue;
            }

            // Re-type array globals.
            // We don't need to worry about the pointer type of the load, as
            // we can instantiate that later.
            if sampled_image.original_uniform_type.class.opcode == spirv::Op::TypeArray {
                let mut op_variable = instr;
                op_variable.result_type = Some(sampled_image.target_texture_pointer_type_id);

                instructions.push(op_variable);
            }
        }

        // replace
        self.builder.module_mut().types_global_values = instructions;
    }

    fn rewrite_loads(
        &mut self,
        combined_image_samplers: &FxHashMap<spirv::Word, CombinedImageSampler>,
    ) {
        let op_type_sampler = self.builder.type_sampler();

        // need to clone
        let mut functions = self.builder.module_ref().functions.clone();

        for function in functions.iter_mut() {
            for block in function.blocks.iter_mut() {
                let mut instructions = Vec::new();
                for instr in block.instructions.drain(..) {
                    if instr.class.opcode != Op::Load {
                        instructions.push(instr);
                        continue;
                    }

                    // This doesn't affect array loads because array loads load the result of the OpAccessChain which can be done in a separate pass.
                    let Some(Operand::IdRef(op_variable_id)) = &instr.operands.get(0) else {
                        instructions.push(instr);
                        continue;
                    };

                    let Some(combined_image_sampler) = combined_image_samplers.get(op_variable_id)
                    else {
                        // Ignore what we didn't process before.
                        instructions.push(instr);
                        continue;
                    };

                    let op_load_texture_id = self.builder.id();
                    let op_load_sampler_id = self.builder.id();

                    let mut load_instr = instr.clone();

                    load_instr.result_type = combined_image_sampler.base_type.result_id;
                    load_instr.result_id = Some(op_load_texture_id);
                    instructions.push(load_instr);

                    // load the sampler
                    instructions.push(Instruction {
                        class: rspirv::grammar::CoreInstructionTable::get(spirv::Op::Load),
                        result_type: Some(op_type_sampler),
                        result_id: Some(op_load_sampler_id),
                        operands: vec![Operand::IdRef(combined_image_sampler.sampler_variable)],
                    });

                    // reuse the old id for the OpSampleImage
                    instructions.push(Instruction {
                        class: rspirv::grammar::CoreInstructionTable::get(spirv::Op::SampledImage),
                        result_type: combined_image_sampler.original_uniform_type.result_id,
                        result_id: instr.result_id,

                        operands: vec![
                            Operand::IdRef(op_load_texture_id),
                            Operand::IdRef(op_load_sampler_id),
                        ],
                    });
                }
                block.instructions = instructions;
            }
        }

        self.builder.module_mut().functions = functions;
    }

    fn collect_op_access_chain<'b>(
        &mut self,
        combined_image_samplers: &'b FxHashMap<spirv::Word, CombinedImageSampler>,
    ) -> FxHashMap<spirv::Word, OpAccessChain<'b>> {
        // need to clone
        let mut functions = self.builder.module_ref().functions.clone();

        let mut seen_op_access_chain = FxHashMap::default();

        for function in functions.iter_mut() {
            for block in function.blocks.iter_mut() {
                let mut instructions = Vec::new();

                // This needs to be done in two passes, first to find the OpAccessChains and
                // update their type to the pointer type of the scalar value,

                // Then to update the OpLoads.
                for instr in block.instructions.clone() {
                    if instr.class.opcode != Op::AccessChain {
                        instructions.push(instr);
                        continue;
                    }

                    // This doesn't affect array loads because array loads load the result of the OpAccessChain which can be done in a separate pass.
                    let Some(Operand::IdRef(op_variable)) = &instr.operands.get(0) else {
                        instructions.push(instr);
                        continue;
                    };

                    // Ensure that this is an OpAccessChain of our combined image sampler.
                    let Some(sampled_image) = combined_image_samplers.get(op_variable) else {
                        // Ignore what we didn't process before.
                        instructions.push(instr);
                        continue;
                    };

                    let Some(original_result_type) = instr.result_type else {
                        // Ignore what we didn't process before.
                        instructions.push(instr);
                        continue;
                    };

                    let Some(index) = instr.operands.get(1).cloned() else {
                        // Ignore what we didn't process before.
                        instructions.push(instr);
                        continue;
                    };

                    let Some(base_type_id) = sampled_image.base_type.result_id else {
                        // Ignore what we didn't process before.
                        instructions.push(instr);
                        continue;
                    };

                    let op_pointer_type_id = self.builder.type_pointer(
                        None,
                        StorageClass::UniformConstant,
                        base_type_id,
                    );

                    let mut op_access_chain = instr;

                    let Some(result_id) = op_access_chain.result_id else {
                        instructions.push(op_access_chain);
                        continue;
                    };

                    op_access_chain.result_type = Some(op_pointer_type_id);

                    instructions.push(op_access_chain);

                    seen_op_access_chain.insert(
                        result_id,
                        OpAccessChain {
                            sampled_image,
                            index,
                            original_result_type,
                            target_pointer_type: op_pointer_type_id,
                        },
                    );
                }

                block.instructions = instructions;
            }
        }

        self.builder.module_mut().functions = functions;
        seen_op_access_chain
    }

    fn rewrite_global_op_access_chain_loads(
        &mut self,
        op_access_chains: &FxHashMap<spirv::Word, OpAccessChain>,
    ) {
        let op_type_sampler = self.builder.type_sampler();
        let op_type_sampler_pointer =
            self.builder
                .type_pointer(None, StorageClass::UniformConstant, op_type_sampler);

        let mut functions = self.builder.module_ref().functions.clone();

        for function in functions.iter_mut() {
            for block in function.blocks.iter_mut() {
                let mut instructions = Vec::new();

                // This needs to be done in two passes, first to find the OpAccessChains and
                // update their type to the pointer type of the scalar value,

                // Then to update the OpLoads.
                for instr in block.instructions.clone() {
                    if instr.class.opcode != Op::Load {
                        instructions.push(instr);
                        continue;
                    }

                    let Some(Operand::IdRef(op_variable)) = &instr.operands.get(0) else {
                        instructions.push(instr);
                        continue;
                    };

                    // Ensure that this is an OpLoad of the previous OpAccessChain
                    let Some(&OpAccessChain {
                        sampled_image,
                        ref index,
                        ..
                    }) = op_access_chains.get(op_variable)
                    else {
                        // Ignore what we didn't process before.
                        instructions.push(instr);
                        continue;
                    };

                    let Some(base_type_id) = sampled_image.base_type.result_id else {
                        // Ignore what we didn't process before.
                        instructions.push(instr);
                        continue;
                    };

                    let op_load_texture_id = self.builder.id();
                    let op_load_sampler_id = self.builder.id();
                    let op_access_chain_sampler_id = self.builder.id();

                    let op_type_sampled_image = self.builder.type_sampled_image(base_type_id);

                    let mut load_instr = instr.clone();

                    load_instr.result_type = sampled_image.base_type.result_id;
                    load_instr.result_id = Some(op_load_texture_id);
                    instructions.push(load_instr);

                    // load the sampler
                    instructions.push(Instruction {
                        class: rspirv::grammar::CoreInstructionTable::get(spirv::Op::AccessChain),
                        result_type: Some(op_type_sampler_pointer),
                        result_id: Some(op_access_chain_sampler_id),
                        operands: vec![
                            Operand::IdRef(sampled_image.sampler_variable),
                            index.clone(),
                        ],
                    });

                    instructions.push(Instruction {
                        class: rspirv::grammar::CoreInstructionTable::get(spirv::Op::Load),
                        result_type: Some(op_type_sampler),
                        result_id: Some(op_load_sampler_id),
                        operands: vec![Operand::IdRef(op_access_chain_sampler_id)],
                    });

                    // reuse the old id for the OpSampleImage
                    instructions.push(Instruction {
                        class: rspirv::grammar::CoreInstructionTable::get(spirv::Op::SampledImage),
                        result_type: Some(op_type_sampled_image),
                        result_id: instr.result_id,

                        operands: vec![
                            Operand::IdRef(op_load_texture_id),
                            Operand::IdRef(op_load_sampler_id),
                        ],
                    });
                }

                block.instructions = instructions;
            }
        }

        self.builder.module_mut().functions = functions;
    }

    // Returns nested hashmap of { OpFunction: { OpType: CombinedImageSampler } },
    // indicating that the listed parameters should change
    fn rewrite_function_calls<'b>(
        &mut self,
        op_access_chains: &'b FxHashMap<spirv::Word, OpAccessChain>,
        combined_image_samplers: &'b FxHashMap<spirv::Word, CombinedImageSampler>,
    ) -> FxHashMap<spirv::Word, FxHashMap<spirv::Word, Cow<'b, CombinedImageSampler>>> {
        let mut seen_functions: FxHashMap<
            spirv::Word,
            FxHashMap<spirv::Word, Cow<'b, CombinedImageSampler>>,
        > = FxHashMap::default();

        // First pass, rewrite function calls involving sampled image access
        for instr in self.builder.module_mut().all_inst_iter_mut() {
            if instr.class.opcode != spirv::Op::FunctionCall {
                continue;
            }

            let Some(&Operand::IdRef(function_id)) = instr.operands.get(0) else {
                continue;
            };

            if !instr.operands[1..].iter().any(|param| {
                let &Operand::IdRef(function_id) = param else {
                    return false;
                };

                combined_image_samplers.contains_key(&function_id)
            }) {
                continue;
            };

            let mut function_call_operands = Vec::with_capacity(instr.operands.len());

            for operand in instr.operands.drain(..) {
                let Operand::IdRef(op_ref_id) = operand else {
                    function_call_operands.push(operand);
                    continue;
                };

                let Some(sampled_image) = combined_image_samplers.get(&op_ref_id) else {
                    function_call_operands.push(operand);
                    continue;
                };

                function_call_operands.push(operand);
                function_call_operands.push(Operand::IdRef(sampled_image.sampler_variable));

                match seen_functions.entry(function_id) {
                    Entry::Occupied(mut vec) => {
                        vec.get_mut().insert(
                            sampled_image.original_uniform_pointer_type_id,
                            Cow::Borrowed(sampled_image),
                        );
                    }
                    Entry::Vacant(vec) => {
                        let mut map = FxHashMap::default();
                        map.insert(
                            sampled_image.original_uniform_pointer_type_id,
                            Cow::Borrowed(sampled_image),
                        );
                        vec.insert(map);
                    }
                }
            }
            instr.operands = function_call_operands;
        }

        // Deal with OpAccessChains.

        let op_type_sampler = self.builder.type_sampler();
        let op_type_sampler_pointer =
            self.builder
                .type_pointer(None, StorageClass::UniformConstant, op_type_sampler);

        let mut functions = self.builder.module_ref().functions.clone();

        for function in functions.iter_mut() {
            for block in function.blocks.iter_mut() {
                let mut instructions = Vec::new();

                // This needs to be done in two passes, first to find the OpAccessChains and
                // update their type to the pointer type of the scalar value,

                // Then to update the OpLoads.
                for mut instr in block.instructions.clone() {
                    if instr.class.opcode != Op::FunctionCall {
                        instructions.push(instr);
                        continue;
                    }

                    let Some(&Operand::IdRef(function_id)) = instr.operands.get(0) else {
                        instructions.push(instr);
                        continue;
                    };

                    // Ensure that this is an OpFunctionCall involving some previous OpAccessChain
                    let relevant_operands = instr.operands[1..]
                        .iter()
                        .filter(|param| {
                            let &Operand::IdRef(op_ref_id) = param else {
                                return false;
                            };

                            op_access_chains.contains_key(&op_ref_id)
                        })
                        .collect::<Vec<_>>();

                    if relevant_operands.is_empty() {
                        instructions.push(instr);
                        continue;
                    }

                    // Maps OpAccessChain of texture to OpAccessChain of sampler
                    let mut op_access_sampler_mapping = FxHashMap::default();
                    // Insert necessary OpAccessChains
                    for operand in relevant_operands {
                        let &Operand::IdRef(op_ref_id) = operand else {
                            continue;
                        };

                        let Some(&OpAccessChain {
                            sampled_image,
                            ref index,
                            ..
                        }) = op_access_chains.get(&op_ref_id)
                        else {
                            continue;
                        };

                        let op_access_chain_sampler_id = self.builder.id();
                        // access chain the sampler
                        instructions.push(Instruction {
                            class: rspirv::grammar::CoreInstructionTable::get(
                                spirv::Op::AccessChain,
                            ),
                            result_type: Some(op_type_sampler_pointer),
                            result_id: Some(op_access_chain_sampler_id),
                            operands: vec![
                                Operand::IdRef(sampled_image.sampler_variable),
                                index.clone(),
                            ],
                        });

                        op_access_sampler_mapping.insert(op_ref_id, op_access_chain_sampler_id);
                    }

                    let mut function_call_operands = Vec::with_capacity(instr.operands.len());
                    for operand in instr.operands.drain(..) {
                        let Operand::IdRef(op_ref_id) = operand else {
                            function_call_operands.push(operand);
                            continue;
                        };

                        let Some(&OpAccessChain {
                            sampled_image,
                            index: _,
                            original_result_type,
                            target_pointer_type,
                        }) = op_access_chains.get(&op_ref_id)
                        else {
                            function_call_operands.push(operand);
                            continue;
                        };

                        let Some(base_type_id) = sampled_image.base_type.result_id else {
                            function_call_operands.push(operand);
                            continue;
                        };

                        let Some(&op_access_chain_sampler) =
                            op_access_sampler_mapping.get(&op_ref_id)
                        else {
                            function_call_operands.push(operand);
                            continue;
                        };

                        function_call_operands.push(operand);
                        function_call_operands.push(Operand::IdRef(op_access_chain_sampler));

                        let Some(original_type) =
                            self.find_global_instruction(original_result_type).cloned()
                        else {
                            continue;
                        };

                        let sampled_image = CombinedImageSampler {
                            sampler_variable: op_access_chain_sampler,
                            sampler_pointer_type: op_type_sampler_pointer,
                            original_uniform_pointer_type_id: original_result_type,
                            original_uniform_type: original_type,
                            target_texture_type_id: base_type_id,
                            target_texture_pointer_type_id: target_pointer_type,
                            base_type: sampled_image.base_type.clone(),
                        };

                        match seen_functions.entry(function_id) {
                            Entry::Occupied(mut vec) => {
                                vec.get_mut()
                                    .insert(original_result_type, Cow::Owned(sampled_image));
                            }
                            Entry::Vacant(vec) => {
                                let mut map = FxHashMap::default();
                                map.insert(original_result_type, Cow::Owned(sampled_image));
                                vec.insert(map);
                            }
                        }
                    }

                    instr.operands = function_call_operands;

                    // Restore the instructions
                    instructions.push(instr);
                }

                block.instructions = instructions;
            }
        }

        self.builder.module_mut().functions = functions;
        seen_functions
    }

    fn rewrite_functions_definitions<'b>(
        &mut self,
        mappings: &FxHashMap<spirv::Word, FxHashMap<spirv::Word, Cow<'b, CombinedImageSampler>>>,
    ) -> FxHashMap<spirv::Word, CombinedImageSampler> {
        let mut sampled_refs = FxHashMap::default();
        let mut functions = self.builder.module_ref().functions.clone();

        for function in functions.iter_mut() {
            let Some(def_id) = function.def_id() else {
                continue;
            };

            if self.seen_functions.contains(&def_id) {
                continue;
            }

            let Some(function_def) = function.def.clone() else {
                continue;
            };

            let Some(param_mappings) = mappings.get(&def_id) else {
                continue;
            };

            let &Some(function_return_type_id) = &function_def.result_type else {
                continue;
            };

            let Some(&Operand::IdRef(_function_type_id)) = &function_def.operands.last() else {
                continue;
            };

            let mut parameters = Vec::new();
            let mut param_types = Vec::new();
            for param in function.parameters.drain(..) {
                let Some(param_id) = param.result_id else {
                    parameters.push(param);
                    continue;
                };

                let Some(param_type) = param.result_type else {
                    parameters.push(param);
                    continue;
                };

                let Some(sampled_image) = param_mappings.get(&param_type) else {
                    parameters.push(param);
                    param_types.push(param_type);
                    continue;
                };

                let op_function_parameter_sampler_id = self.builder.id();

                parameters.push(Instruction {
                    class: rspirv::grammar::CoreInstructionTable::get(spirv::Op::FunctionParameter),
                    result_type: Some(sampled_image.target_texture_pointer_type_id),
                    result_id: param.result_id,
                    operands: vec![],
                });

                parameters.push(Instruction {
                    class: rspirv::grammar::CoreInstructionTable::get(spirv::Op::FunctionParameter),
                    result_type: Some(sampled_image.sampler_pointer_type),
                    result_id: Some(op_function_parameter_sampler_id),
                    operands: vec![],
                });

                param_types.push(sampled_image.target_texture_pointer_type_id);
                param_types.push(sampled_image.sampler_pointer_type);

                sampled_refs.insert(
                    param_id,
                    CombinedImageSampler {
                        sampler_variable: op_function_parameter_sampler_id,
                        sampler_pointer_type: sampled_image.sampler_pointer_type,
                        original_uniform_pointer_type_id: param_type,
                        original_uniform_type: sampled_image.original_uniform_type.clone(),
                        target_texture_type_id: sampled_image.target_texture_type_id,
                        target_texture_pointer_type_id: sampled_image
                            .target_texture_pointer_type_id,
                        base_type: sampled_image.base_type.clone(),
                    },
                );
            }

            let new_type = self
                .builder
                .type_function(function_return_type_id, param_types);

            if let Some(function) = &mut function.def {
                if let Some(function_type) = function.operands.last_mut() {
                    *function_type = Operand::IdRef(new_type);
                }
            }

            function.parameters = parameters;
            self.seen_functions.insert(def_id);
        }

        self.builder.module_mut().functions = functions;

        sampled_refs
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use naga::back::wgsl::WriterFlags;
    use naga::front::spv::Options;
    use naga::valid::{Capabilities, ValidationFlags};
    use rspirv::binary::{Assemble, Disassemble};
    use std::fs::File;
    use std::io::{Read, Write};
    use std::path::Path;

    fn check_wgsl(path: impl AsRef<Path>) {
        let mut bin = Vec::new();

        File::open(path).unwrap().read_to_end(&mut bin).unwrap();

        let mut loader = rspirv::dr::Loader::new();
        rspirv::binary::parse_bytes(bin, &mut loader).unwrap();
        let module = loader.module();
        let mut builder = Builder::new_from_module(module);

        let mut pass = LowerCombinedImageSamplerPass::new(&mut builder);

        pass.ensure_op_type_sampler();
        pass.do_pass();
        // find_op_type_sampled_image(&builder.builder.module_ref());

        println!("{}", pass.builder.module_ref().disassemble());

        let spirv = builder.module().assemble();

        File::create("out.spv")
            .unwrap()
            .write_all(bytemuck::cast_slice(&spirv))
            .unwrap();

        let mut module = naga::front::spv::parse_u8_slice(
            bytemuck::cast_slice(&spirv),
            &Options {
                adjust_coordinate_space: false,
                strict_capabilities: false,
                block_ctx_dump_prefix: None,
            },
        )
        .unwrap();

        let images = module
            .global_variables
            .iter()
            .filter(|&(_, gv)| {
                let ty = &module.types[gv.ty];
                match ty.inner {
                    naga::TypeInner::Image { .. } => true,
                    naga::TypeInner::BindingArray { base, .. } => {
                        let ty = &module.types[base];
                        matches!(ty.inner, naga::TypeInner::Image { .. })
                    }
                    _ => false,
                }
            })
            .map(|(_, gv)| (gv.binding.clone(), gv.space))
            .collect::<naga::FastHashSet<_>>();

        module
            .global_variables
            .iter_mut()
            .filter(|(_, gv)| {
                let ty = &module.types[gv.ty];
                match ty.inner {
                    naga::TypeInner::Sampler { .. } => true,
                    naga::TypeInner::BindingArray { base, .. } => {
                        let ty = &module.types[base];
                        matches!(ty.inner, naga::TypeInner::Sampler { .. })
                    }
                    _ => false,
                }
            })
            .for_each(|(_, gv)| {
                if images.contains(&(gv.binding.clone(), gv.space)) {
                    if let Some(binding) = &mut gv.binding {
                        binding.group = 1;
                    }
                }
            });

        let mut valid = naga::valid::Validator::new(ValidationFlags::all(), Capabilities::empty());
        let info = valid.validate(&module).unwrap();

        let wgsl =
            naga::back::wgsl::write_string(&module, &info, WriterFlags::EXPLICIT_TYPES).unwrap();

        println!("{}", wgsl);
    }

    #[test]
    fn it_works_known() {
        check_wgsl("./test/combined-image-sampler.spv");
        check_wgsl("./test/combined-image-sampler-array.spv");
        check_wgsl("./test/function-call-single-scalar-sampler.spv");
    }

    #[test]
    fn it_works() {
        // check_wgsl("./test/combined-image-sampler.spv");
        // check_wgsl("./test/combined-image-sampler-array.spv");
        // check_wgsl("./test/function-call-single-scalar-sampler.spv");

        check_wgsl("./test/access_out_of_call.spv");
    }
}
