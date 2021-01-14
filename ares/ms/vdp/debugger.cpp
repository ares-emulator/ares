auto VDP::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("VDP VRAM");
  memory.vram->setSize(vdp.vram.size());
  memory.vram->setRead([&](u32 address) -> u8 {
    return vdp.vram[address];
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    vdp.vram[address] = data;
  });

  memory.cram = parent->append<Node::Debugger::Memory>("VDP CRAM");
  memory.cram->setSize(vdp.cram.size());
  memory.cram->setRead([&](u32 address) -> u8 {
    return vdp.cram[address];
  });
  memory.cram->setWrite([&](u32 address, u8 data) -> void {
    vdp.cram[address] = data;
  });
}
