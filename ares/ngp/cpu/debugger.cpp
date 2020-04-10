auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("CPU RAM");
  memory.ram->setSize(cpu.ram.size());
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return cpu.ram[address];
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.ram[address] = data;
  });

  tracer.instruction = parent->append<Node::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.r.pc.l.l0)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
