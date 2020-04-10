auto PPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Memory>("PPU VRAM");
  memory.vram->setSize(ppu.vram.size());
  memory.vram->setRead([&](uint32 address) -> uint8 {
    return ppu.vram[address];
  });
  memory.vram->setWrite([&](uint32 address, uint8 data) -> void {
    ppu.vram[address] = data;
  });

  memory.oam = parent->append<Node::Memory>("PPU OAM");
  memory.oam->setSize(ppu.oam.size());
  memory.oam->setRead([&](uint32 address) -> uint8 {
    return ppu.oam[address];
  });
  memory.oam->setWrite([&](uint32 address, uint8 data) -> void {
    ppu.oam[address] = data;
  });
}
