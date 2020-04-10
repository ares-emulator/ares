auto VDP::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Memory>("VDP VRAM");
  memory.vram->setSize(32_KiB << 1);
  memory.vram->setRead([&](uint32 address) -> uint8 {
    return vdp.vram.memory[uint15(address >> 1)].byte(!address.bit(0));
  });
  memory.vram->setWrite([&](uint32 address, uint8 data) -> void {
    vdp.vram.memory[uint15(address >> 1)].byte(!address.bit(0)) = data;
  });

  memory.vsram = parent->append<Node::Memory>("VDP VSRAM");
  memory.vsram->setSize(40 << 1);
  memory.vsram->setRead([&](uint32 address) -> uint8 {
    if(address >= 40 << 1) return 0x00;
    return vdp.vsram.memory[address >> 1].byte(!address.bit(0));
  });
  memory.vsram->setWrite([&](uint32 address, uint8 data) -> void {
    if(address >= 40 << 1) return;
    vdp.vsram.memory[address >> 1].byte(!address.bit(0)) = data;
  });

  memory.cram = parent->append<Node::Memory>("VDP CRAM");
  memory.cram->setSize(64 << 1);
  memory.cram->setRead([&](uint32 address) -> uint8 {
    return vdp.cram.memory[uint6(address >> 1)].byte(!address.bit(0));
  });
  memory.cram->setWrite([&](uint32 address, uint8 data) -> void {
    vdp.cram.memory[uint6(address >> 1)].byte(!address.bit(0)) = data;
  });
}
