auto PPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("PPU VRAM");
  memory.vram->setSize(ppu.vram.size());
  memory.vram->setRead([&](u32 address) -> u8 {
    return ppu.vram[address];
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    ppu.vram[address] = data;
  });

  memory.pram = parent->append<Node::Debugger::Memory>("PPU PRAM");
  memory.pram->setSize(ppu.pram.size());
  memory.pram->setRead([&](u32 address) -> u8 {
    return ppu.pram[address >> 1].byte(address & 1);
  });
  memory.pram->setWrite([&](u32 address, u8 data) -> void {
    ppu.pram[address >> 1].byte(address & 1) = data;
  });
}
