auto Aleck64::Debugger::load(Node::Object parent) -> void {

  memory.sdram = parent->append<Node::Debugger::Memory>("Aleck64 SDRAM");
  memory.sdram->setSize(4_MiB );
  memory.sdram->setRead([&](u32 address) -> u8 {
    return aleck64.sdram.read<Byte>(address);
  });
  memory.sdram->setWrite([&](u32 address, u8 data) -> void {
    return aleck64.sdram.write<Byte>(address, data);
  });

  memory.vram = parent->append<Node::Debugger::Memory>("Aleck64 VRAM");
  memory.vram->setSize(4_KiB);
  memory.vram->setRead([&](u32 address) -> u8 {
    return aleck64.vram.read<Byte>(address);
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    return aleck64.vram.write<Byte>(address, data);
  });

  memory.pram = parent->append<Node::Debugger::Memory>("Aleck64 PRAM");
  memory.pram->setSize(4_KiB);
  memory.pram->setRead([&](u32 address) -> u8 {
    return aleck64.pram.read<Byte>(address);
  });
  memory.pram->setWrite([&](u32 address, u8 data) -> void {
    return aleck64.pram.write<Byte>(address, data);
  });
}

auto Aleck64::Debugger::unload() -> void {
  memory.sdram.reset();
  memory.vram.reset();
  memory.pram.reset();
}
