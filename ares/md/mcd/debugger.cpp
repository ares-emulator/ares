auto MCD::Debugger::load(Node::Object parent) -> void {
  memory.pram = parent->append<Node::Debugger::Memory>("CD PRAM");
  memory.pram->setSize(256_KiB << 1);
  memory.pram->setRead([&](u32 address) -> u8 {
    return mcd.pram[address >> 1].byte(!(address & 1));
  });
  memory.pram->setWrite([&](u32 address, u8 data) -> void {
    mcd.pram[address >> 1].byte(!(address & 1)) = data;
  });

  memory.wram = parent->append<Node::Debugger::Memory>("CD WRAM");
  memory.wram->setSize(128_KiB << 1);
  memory.wram->setRead([&](u32 address) -> u8 {
    return mcd.wram[address >> 1].byte(!(address & 1));
  });
  memory.wram->setWrite([&](u32 address, u8 data) -> void {
    mcd.wram[address >> 1].byte(!(address & 1)) = data;
  });

  memory.bram = parent->append<Node::Debugger::Memory>("CD BRAM");
  memory.bram->setSize(8_KiB);
  memory.bram->setRead([&](u32 address) -> u8 {
    return mcd.bram[address];
  });
  memory.bram->setWrite([&](u32 address, u8 data) -> void {
    mcd.bram[address] = data;
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "MCD");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "MCD");
}

auto MCD::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(mcd.r.pc - 4)) {
    tracer.instruction->notify(mcd.disassembleInstruction(mcd.r.pc - 4), mcd.disassembleContext());
  }
}

auto MCD::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
