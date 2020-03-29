auto SA1::Debugger::load(Node::Object parent, Node::Object from) -> void {
  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "SA1");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "SA1");
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
