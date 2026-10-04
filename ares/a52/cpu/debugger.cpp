auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.bus = parent->append<Node::Debugger::Memory>("CPU Bus");
  memory.bus->setSize(64_KiB);
  memory.bus->setRead([&](u32 address) -> u8 {
    return cpu.readDebugger(address);
  });

  memory.ram = parent->append<Node::Debugger::Memory>("CPU RAM");
  memory.ram->setSize(system.ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return system.ram.read(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    system.ram.write(address, data);
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(16);
  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.PC)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) tracer.interrupt->notify(type);
}
