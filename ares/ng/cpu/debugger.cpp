auto CPU::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(unlikely(tracer.instruction->enabled())) {
    if(tracer.instruction->address(cpu.r.pc - 4)) {
      tracer.instruction->notify(cpu.disassembleInstruction(cpu.r.pc - 4), cpu.disassembleContext());
    }
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(unlikely(tracer.interrupt->enabled())) {
    string message = {type, " SR=", cpu.r.i, " @ ", lspc.io.vcounter, ",", lspc.io.hcounter};
    tracer.interrupt->notify(message);
  }
}
