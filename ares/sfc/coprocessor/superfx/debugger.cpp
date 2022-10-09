auto SuperFX::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "GSU");
  tracer.instruction->setAddressBits(24);
  tracer.instruction->setDepth(superfx.Frequency < 12_MHz ? 8 : 16);
}

auto SuperFX::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(superfx.regs.r[15])) {
    tracer.instruction->notify(superfx.disassembleInstruction(), superfx.disassembleContext());
  }
}
