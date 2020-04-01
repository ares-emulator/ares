auto CPU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  memory.iwram = Node::append<Node::Memory>(parent, from, "CPU IWRAM");
  memory.iwram->setSize(cpu.iwram.size());
  memory.iwram->setRead([&](uint32 address) -> uint8 {
    return cpu.iwram[address];
  });
  memory.iwram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.iwram[address] = data;
  });

  memory.ewram = Node::append<Node::Memory>(parent, from, "CPU EWRAM");
  memory.ewram->setSize(cpu.ewram.size());
  memory.ewram->setRead([&](uint32 address) -> uint8 {
    return cpu.ewram[address];
  });
  memory.ewram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.ewram[address] = data;
  });

  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  tracer.instruction->setAddressBits(32);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.pipeline.execute.address)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
