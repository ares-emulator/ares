auto NECDSP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "NEC");
  tracer.instruction->setAddressBits(14);
  tracer.instruction->setDepth(necdsp.Frequency < 12_MHz ? 8 : 16);
}

auto NECDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(necdsp.regs.pc)) {
    tracer.instruction->notify(necdsp.disassembleInstruction(), necdsp.disassembleContext());
  }
}
