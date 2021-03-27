auto SVP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "SVP");
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "SVP");
}

auto SVP::Debugger::unload(Node::Object parent) -> void {
  parent->remove(tracer.instruction);
  parent->remove(tracer.interrupt);
  tracer.instruction.reset();
  tracer.interrupt.reset();
}

auto SVP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(super->PC)) {
    tracer.instruction->notify(super->disassembleInstruction(), super->disassembleContext());
  }
}

auto SVP::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
