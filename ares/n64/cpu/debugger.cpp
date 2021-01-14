auto CPU::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(32, 2);

  tracer.exception = parent->append<Node::Debugger::Tracer::Notification>("Exception", "CPU");
  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::unload() -> void {
  tracer.instruction.reset();
  tracer.exception.reset();
  tracer.interrupt.reset();
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled()) {
    u32 address = cpu.pipeline.address;
    u32 instruction = cpu.pipeline.instruction;
    if(tracer.instruction->address(address)) {
      cpu.disassembler.showColors = 0;
      tracer.instruction->notify(cpu.disassembler.disassemble(address, instruction), {});
      cpu.disassembler.showColors = 1;
    }
  }
}

auto CPU::Debugger::exception(string_view type) -> void {
  if(tracer.exception->enabled()) {
    tracer.exception->notify(type);
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
