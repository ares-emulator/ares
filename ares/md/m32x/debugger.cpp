auto M32X::Debugger::load(Node::Object parent) -> void {
  memory.dram = parent->append<Node::Debugger::Memory>("32X DRAM");
  memory.dram->setSize(m32x.dram.size() << 1);
  memory.dram->setRead([&](u32 address) -> u8 {
    return m32x.dram[address >> 1].byte(!(address & 1));
  });
  memory.dram->setWrite([&](u32 address, u8 data) -> void {
    m32x.dram[address >> 1].byte(!(address & 1)) = data;
  });

  memory.sdram = parent->append<Node::Debugger::Memory>("32X SDRAM");
  memory.sdram->setSize(m32x.sdram.size() << 1);
  memory.sdram->setRead([&](u32 address) -> u8 {
    return m32x.sdram[address >> 1].byte(!(address & 1));
  });
  memory.sdram->setWrite([&](u32 address, u8 data) -> void {
    m32x.sdram[address >> 1].byte(!(address & 1)) = data;
  });

  memory.cram = parent->append<Node::Debugger::Memory>("32X CRAM");
  memory.cram->setSize(m32x.cram.size() << 1);
  memory.cram->setRead([&](u32 address) -> u8 {
    return m32x.cram[address >> 1].byte(!(address & 1));
  });
  memory.cram->setWrite([&](u32 address, u8 data) -> void {
    m32x.cram[address >> 1].byte(!(address & 1)) = data;
  });
}

auto M32X::SHM::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "SHM");
  tracer.instruction->setAddressBits(32, 1);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "SHM");
}

auto M32X::SHM::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(m32x.shm.PC - 4)) {
    tracer.instruction->notify(m32x.shm.disassembleInstruction(), m32x.shm.disassembleContext());
  }
}

auto M32X::SHM::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}

auto M32X::SHS::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "SHS");
  tracer.instruction->setAddressBits(32, 1);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "SHS");
}

auto M32X::SHS::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(m32x.shs.PC - 4)) {
    tracer.instruction->notify(m32x.shs.disassembleInstruction(), m32x.shs.disassembleContext());
  }
}

auto M32X::SHS::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
