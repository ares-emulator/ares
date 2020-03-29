auto MCD::Debugger::load(Node::Object parent, Node::Object from) -> void {
  memory.pram = Node::append<Node::Memory>(parent, from, "Mega CD PRAM");
  memory.pram->setSize(256_KiB << 1);
  memory.pram->setRead([&](uint32 address) -> uint8 {
    return mcd.pram[address >> 1].byte(!address.bit(0));
  });
  memory.pram->setWrite([&](uint32 address, uint8 data) -> void {
    mcd.pram[address >> 1].byte(!address.bit(0)) = data;
  });

  memory.wram = Node::append<Node::Memory>(parent, from, "Mega CD WRAM");
  memory.wram->setSize(128_KiB << 1);
  memory.wram->setRead([&](uint32 address) -> uint8 {
    return mcd.wram[address >> 1].byte(!address.bit(0));
  });
  memory.wram->setWrite([&](uint32 address, uint8 data) -> void {
    mcd.wram[address >> 1].byte(!address.bit(0)) = data;
  });

  memory.bram = Node::append<Node::Memory>(parent, from, "Mega CD BRAM");
  memory.bram->setSize(8_KiB);
  memory.bram->setRead([&](uint32 address) -> uint8 {
    return mcd.bram[address];
  });
  memory.bram->setWrite([&](uint32 address, uint8 data) -> void {
    mcd.bram[address] = data;
  });

  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "MCD");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "MCD");
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
