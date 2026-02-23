auto SA1::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(sa1.rom.size());
  memory.rom->setRead([&](u32 address) -> u8 {
    return sa1.rom.read(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return sa1.rom.program(address, data);
  });

  if(sa1.bwram) {
    memory.bwram = parent->append<Node::Debugger::Memory>("SA1 BWRAM");
    memory.bwram->setSize(sa1.bwram.size());
    memory.bwram->setRead([&](u32 address) -> u8 {
      return sa1.bwram.read(address);
    });
    memory.bwram->setWrite([&](u32 address, u8 data) -> void {
      return sa1.bwram.write(address, data);
    });
  }

  if(sa1.iram) {
    memory.iram = parent->append<Node::Debugger::Memory>("SA1 IRAM");
    memory.iram->setSize(sa1.iram.size());
    memory.iram->setRead([&](u32 address) -> u8 {
      return sa1.iram.read(address);
    });
    memory.iram->setWrite([&](u32 address, u8 data) -> void {
      return sa1.iram.write(address, data);
    });
  }

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "SA1");
  tracer.instruction->setAddressBits(24);
  tracer.instruction->setDepth(8);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "SA1");
}

auto SA1::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(sa1.r.pc.d)) {
    tracer.instruction->notify(sa1.disassembleInstruction(), sa1.disassembleContext());
  }
}

auto SA1::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
