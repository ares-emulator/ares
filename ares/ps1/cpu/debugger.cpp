auto CPU::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(32);

  tracer.exception = parent->append<Node::Notification>("Exception", "CPU");
  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "CPU");

  memory.ram = parent->append<Node::Memory>("CPU RAM");
  memory.ram->setSize(cpu.ram.size);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return cpu.ram.readByte(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return cpu.ram.writeByte(address, data);
  });

  memory.cache = parent->append<Node::Memory>("CPU Cache");
  memory.cache->setSize(cpu.cache.size);
  memory.cache->setRead([&](uint32 address) -> uint8 {
    return cpu.cache.readByte(address);
  });
  memory.cache->setWrite([&](uint32 address, uint8 data) -> void {
    return cpu.cache.writeByte(address, data);
  });
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
