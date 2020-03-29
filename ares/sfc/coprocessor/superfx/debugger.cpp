auto SuperFX::Debugger::load(Node::Object parent, Node::Object from) -> void {
  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "GSU");
  tracer.instruction->setAddressBits(24);
}

auto SuperFX::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(superfx.regs.r[15])) {
    tracer.instruction->notify(superfx.disassembleInstruction(), superfx.disassembleContext());
  }
}
