auto SVP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "SVP");
  tracer.instruction->setAddressBits(16);
}

auto SVP::Debugger::unload(Node::Object parent) -> void {
  parent->remove(tracer.instruction);
  tracer.instruction.reset();
}

auto SVP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(super->PC)) {
    tracer.instruction->notify(super->disassembleInstruction(), super->disassembleContext());
  }
}
