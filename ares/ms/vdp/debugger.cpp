auto VDP::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Memory>("VDP VRAM");
  memory.vram->setSize(vdp.vram.size());
  memory.vram->setRead([&](uint32 address) -> uint8 {
    return vdp.vram[address];
  });
  memory.vram->setWrite([&](uint32 address, uint8 data) -> void {
    vdp.vram[address] = data;
  });

  memory.cram = parent->append<Node::Memory>("VDP CRAM");
  memory.cram->setSize(vdp.cram.size());
  memory.cram->setRead([&](uint32 address) -> uint8 {
    return vdp.cram[address];
  });
  memory.cram->setWrite([&](uint32 address, uint8 data) -> void {
    vdp.cram[address] = data;
  });
}
