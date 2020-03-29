auto CPU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  string origin = Model::SuperGameBoy() ? "SGB" : "CPU";

  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", origin);
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", origin);
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.PC)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
