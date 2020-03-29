auto SMP::Debugger::load(Node::Object parent, Node::Object from) -> void {
  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "SMP");
  tracer.instruction->setAddressBits(16);
}

auto SMP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(smp.r.pc.w)) {
    tracer.instruction->notify(smp.disassembleInstruction(), smp.disassembleContext());
  }
}
