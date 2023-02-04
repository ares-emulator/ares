auto HitachiDSP::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(hitachidsp.rom.size());
  memory.rom->setRead([&](u32 address) -> u8 {
    return hitachidsp.rom.read(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return hitachidsp.rom.program(address, data);
  });

  if(hitachidsp.ram) {
    memory.ram = parent->append<Node::Debugger::Memory>("Cartridge RAM");
    memory.ram->setSize(hitachidsp.ram.size());
    memory.ram->setRead([&](u32 address) -> u8 {
      return hitachidsp.ram.read(address);
    });
    memory.ram->setWrite([&](u32 address, u8 data) -> void {
      return hitachidsp.ram.write(address, data);
    });
  }

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "HIT");
  tracer.instruction->setAddressBits(23);
  tracer.instruction->setDepth(16);
}

auto HitachiDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(hitachidsp.r.pb << 8 | hitachidsp.r.pc)) {
    tracer.instruction->notify(hitachidsp.disassembleInstruction(), hitachidsp.disassembleContext());
  }
}
