auto M32X::SHM::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "SHM");
  tracer.instruction->setAddressBits(29);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "SHM");
}

auto M32X::SHM::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(m32x.shm.PC & 0x1fff'ffff)) {
    tracer.instruction->notify(m32x.shm.disassembleInstruction(), m32x.shm.disassembleContext());
  }
}

auto M32X::SHM::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}

auto M32X::SHS::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "SHS");
  tracer.instruction->setAddressBits(29);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "SHS");
}

auto M32X::SHS::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(m32x.shs.PC & 0x1fff'ffff)) {
    tracer.instruction->notify(m32x.shs.disassembleInstruction(), m32x.shs.disassembleContext());
  }
}

auto M32X::SHS::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
