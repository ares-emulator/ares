auto SMP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Instruction>("Instruction", "SMP");
  tracer.instruction->setAddressBits(16);
}

auto SMP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(smp.r.pc.w)) {
    tracer.instruction->notify(smp.disassembleInstruction(), smp.disassembleContext());
  }
}
