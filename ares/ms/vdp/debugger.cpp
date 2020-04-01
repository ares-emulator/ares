auto VDP::Debugger::load(Node::Object parent, Node::Object from) -> void {
  memory.vram = Node::append<Node::Memory>(parent, from, "VDP VRAM");
  memory.vram->setSize(vdp.vram.size());
  memory.vram->setRead([&](uint32 address) -> uint8 {
    return vdp.vram[address];
  });
  memory.vram->setWrite([&](uint32 address, uint8 data) -> void {
    vdp.vram[address] = data;
  });

  memory.cram = Node::append<Node::Memory>(parent, from, "VDP CRAM");
  memory.cram->setSize(vdp.cram.size());
  memory.cram->setRead([&](uint32 address) -> uint8 {
    return vdp.cram[address];
  });
  memory.cram->setWrite([&](uint32 address, uint8 data) -> void {
    vdp.cram[address] = data;
  });
}
