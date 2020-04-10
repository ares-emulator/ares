auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("CPU RAM");
  memory.ram->setSize(32_KiB << 1);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return cpu.ram[uint15(address >> 1)].byte(!address.bit(0));
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.ram[uint15(address >> 1)].byte(!address.bit(0)) = data;
  });

  tracer.instruction = parent->append<Node::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.r.pc - 4)) {
    tracer.instruction->notify(cpu.disassembleInstruction(cpu.r.pc - 4), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
