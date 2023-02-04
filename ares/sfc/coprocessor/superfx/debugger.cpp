auto SuperFX::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(superfx.rom.size());
  memory.rom->setRead([&](u32 address) -> u8 {
    return superfx.rom.read(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return superfx.rom.program(address, data);
  });

  if(superfx.ram) {
    memory.ram = parent->append<Node::Debugger::Memory>("SuperFX RAM");
    memory.ram->setSize(superfx.ram.size());
    memory.ram->setRead([&](u32 address) -> u8 {
      return superfx.ram.read(address);
    });
    memory.ram->setWrite([&](u32 address, u8 data) -> void {
      return superfx.ram.write(address, data);
    });
  }

  if(superfx.bram) {
    memory.bram = parent->append<Node::Debugger::Memory>("SuperFX BRAM");
    memory.bram->setSize(superfx.bram.size());
    memory.bram->setRead([&](u32 address) -> u8 {
      return superfx.bram.read(address);
    });
    memory.bram->setWrite([&](u32 address, u8 data) -> void {
      return superfx.bram.write(address, data);
    });
  }

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "GSU");
  tracer.instruction->setAddressBits(24);
  tracer.instruction->setDepth(superfx.Frequency < 12_MHz ? 8 : 16);
}

auto SuperFX::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(superfx.regs.r[15])) {
    tracer.instruction->notify(superfx.disassembleInstruction(), superfx.disassembleContext());
  }
}
