auto APU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "APU");
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "APU");
}

auto APU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(apu.r.pc)) {
    tracer.instruction->notify(apu.disassembleInstruction(), apu.disassembleContext());
  }
}

auto APU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
