auto PPU::Debugger::load(Node::Object parent) -> void {
  memory.ciram = parent->append<Node::Debugger::Memory>("PPU CIRAM");
  memory.ciram->setSize(ppu.ciram.size());
  memory.ciram->setRead([&](u32 address) -> u8 {
    return ppu.ciram[address];
  });
  memory.ciram->setWrite([&](u32 address, u8 data) -> void {
    ppu.ciram[address] = data;
  });

  memory.cgram = parent->append<Node::Debugger::Memory>("PPU CGRAM");
  memory.cgram->setSize(ppu.cgram.size());
  memory.cgram->setRead([&](u32 address) -> u8 {
    return ppu.cgram[address];
  });
  memory.cgram->setWrite([&](u32 address, u8 data) -> void {
    ppu.cgram[address] = data;
  });

  memory.oam = parent->append<Node::Debugger::Memory>("PPU OAM");
  memory.oam->setSize(ppu.oam.size());
  memory.oam->setRead([&](u32 address) -> u8 {
    return ppu.oam[address];
  });
  memory.oam->setWrite([&](u32 address, u8 data) -> void {
    ppu.oam[address] = data;
  });
}

auto PPU::Debugger::unload() -> void {
  memory.ciram.reset();
  memory.cgram.reset();
  memory.oam.reset();
}
