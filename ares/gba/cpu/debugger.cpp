auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.iwram = parent->append<Node::Debugger::Memory>("CPU IWRAM");
  memory.iwram->setSize(cpu.iwram.size());
  memory.iwram->setRead([&](u32 address) -> u8 {
    return cpu.iwram[address];
  });
  memory.iwram->setWrite([&](u32 address, u8 data) -> void {
    cpu.iwram[address] = data;
  });

  memory.ewram = parent->append<Node::Debugger::Memory>("CPU EWRAM");
  memory.ewram->setSize(cpu.ewram.size());
  memory.ewram->setRead([&](u32 address) -> u8 {
    return cpu.ewram[address];
  });
  memory.ewram->setWrite([&](u32 address, u8 data) -> void {
    cpu.ewram[address] = data;
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(32);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
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
