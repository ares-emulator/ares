auto Cartridge::Debugger::load(Node::Object parent) -> void {
  if(self.rom) {
    memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
    memory.rom->setSize(self.rom.size());
    memory.rom->setRead([&](u32 address) -> u8 {
      return self.rom.read(address);
    });
    memory.rom->setWrite([&](u32 address, u8 data) -> void {
      return self.rom.program(address, data);
    });
  }

  if(self.ram) {
    memory.ram = parent->append<Node::Debugger::Memory>("Cartridge RAM");
    memory.ram->setSize(self.ram.size());
    memory.ram->setRead([&](u32 address) -> u8 {
      return self.ram.read(address);
    });
    memory.ram->setWrite([&](u32 address, u8 data) -> void {
      return self.ram.write(address, data);
    });
  }

  if(self.eeprom) {
    memory.eeprom = parent->append<Node::Debugger::Memory>("Cartridge EEPROM");
    memory.eeprom->setSize(self.eeprom.size);
    memory.eeprom->setRead([&](u32 address) -> u8 {
      return self.eeprom.data[address];
    });
    memory.eeprom->setWrite([&](u32 address, u8 data) -> void {
      self.eeprom.data[address] = data;
    });
  }
}

auto Cartridge::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.rom);
  parent->remove(memory.ram);
  parent->remove(memory.eeprom);
  memory.rom.reset();
  memory.ram.reset();
  memory.eeprom.reset();
}
