auto PPU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  memory.vram = Node::append<Node::Memory>(parent, from, "PPU VRAM");
  memory.vram->setSize(ppu.vram.size());
  memory.vram->setRead([&](uint32 address) -> uint8 {
    return ppu.vram[address];
  });
  memory.vram->setWrite([&](uint32 address, uint8 data) -> void {
    ppu.vram[address] = data;
  });

  memory.pram = Node::append<Node::Memory>(parent, from, "PPU PRAM");
  memory.pram->setSize(ppu.pram.size());
  memory.pram->setRead([&](uint32 address) -> uint8 {
    return ppu.pram[address >> 1].byte(address & 1);
  });
  memory.pram->setWrite([&](uint32 address, uint8 data) -> void {
    ppu.pram[address >> 1].byte(address & 1) = data;
  });
}
