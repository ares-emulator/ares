auto CPU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  memory.ram = Node::append<Node::Memory>(parent, from, "CPU RAM");
  memory.ram->setSize(cpu.ram.size());
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return cpu.ram[address];
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.ram[address] = data;
  });

  memory.expansion = Node::append<Node::Memory>(parent, from, "CPU EXPRAM");
  memory.expansion->setSize(cpu.expansion.size());
  memory.expansion->setRead([&](uint32 address) -> uint8 {
    return cpu.expansion[address];
  });
  memory.expansion->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.expansion[address] = data;
  });

  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.r.pc)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
