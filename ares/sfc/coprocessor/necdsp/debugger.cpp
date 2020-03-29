auto NECDSP::Debugger::load(Node::Object parent, Node::Object from) -> void {
  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "NEC");
  tracer.instruction->setAddressBits(14);
}

auto NECDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(necdsp.regs.pc)) {
    tracer.instruction->notify(necdsp.disassembleInstruction(), necdsp.disassembleContext());
  }
}
