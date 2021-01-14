auto Cartridge::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(cartridge.rom.size());
  memory.rom->setRead([&](u32 address) -> u8 {
    return cartridge.rom.read(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return cartridge.rom.program(address, data);
  });

  memory.ram = parent->append<Node::Debugger::Memory>("Cartridge RAM");
  memory.ram->setSize(cartridge.ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return cartridge.ram.read(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return cartridge.ram.write(address, data);
  });
}
