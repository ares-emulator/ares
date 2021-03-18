auto M32X::Debugger::load(Node::Object parent) -> void {
  memory.sdram = parent->append<Node::Debugger::Memory>("32X SDRAM");
  memory.sdram->setSize(m32x.sdram.size() << 1);
  memory.sdram->setRead([&](u32 address) -> u8 {
    return m32x.sdram[address >> 1].byte(!(address & 1));
  });
  memory.sdram->setWrite([&](u32 address, u8 data) -> void {
    m32x.sdram[address >> 1].byte(!(address & 1)) = data;
  });
}

auto M32X::SH7604::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", parent->name());
  tracer.instruction->setAddressBits(32, 1);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", parent->name());
}

auto M32X::SH7604::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(self->PC - 4)) {
    tracer.instruction->notify(self->disassembleInstruction(), self->disassembleContext());
  }
}

auto M32X::SH7604::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}

auto M32X::VDP::Debugger::load(Node::Object parent) -> void {
  memory.dram = parent->append<Node::Debugger::Memory>("32X DRAM");
  memory.dram->setSize(m32x.vdp.dram.size() << 1);
  memory.dram->setRead([&](u32 address) -> u8 {
    return m32x.vdp.dram[address >> 1].byte(!(address & 1));
  });
  memory.dram->setWrite([&](u32 address, u8 data) -> void {
    m32x.vdp.dram[address >> 1].byte(!(address & 1)) = data;
  });

  memory.cram = parent->append<Node::Debugger::Memory>("32X CRAM");
  memory.cram->setSize(m32x.vdp.cram.size() << 1);
  memory.cram->setRead([&](u32 address) -> u8 {
    return m32x.vdp.cram[address >> 1].byte(!(address & 1));
  });
  memory.cram->setWrite([&](u32 address, u8 data) -> void {
    m32x.vdp.cram[address >> 1].byte(!(address & 1)) = data;
  });
}
