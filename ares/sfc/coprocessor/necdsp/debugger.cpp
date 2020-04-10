auto NECDSP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Instruction>("Instruction", "NEC");
  tracer.instruction->setAddressBits(14);
}

auto NECDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(necdsp.regs.pc)) {
    tracer.instruction->notify(necdsp.disassembleInstruction(), necdsp.disassembleContext());
  }
}
