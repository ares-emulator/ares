auto SuperFX::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Instruction>("Instruction", "GSU");
  tracer.instruction->setAddressBits(24);
}

auto SuperFX::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(superfx.regs.r[15])) {
    tracer.instruction->notify(superfx.disassembleInstruction(), superfx.disassembleContext());
  }
}
