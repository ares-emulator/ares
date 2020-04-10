auto SA1::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Instruction>("Instruction", "SA1");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "SA1");
}

auto SA1::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(sa1.r.pc.d)) {
    tracer.instruction->notify(sa1.disassembleInstruction(), sa1.disassembleContext());
  }
}

auto SA1::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
