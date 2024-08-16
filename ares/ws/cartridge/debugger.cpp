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

  if(self.has.sram) {
    memory.ram = parent->append<Node::Debugger::Memory>("Cartridge RAM");
    memory.ram->setSize(self.ram.size());
    memory.ram->setRead([&](u32 address) -> u8 {
      return self.ram.read(address);
    });
    memory.ram->setWrite([&](u32 address, u8 data) -> void {
      return self.ram.write(address, data);
    });
  }

  if(self.has.eeprom) {
    memory.eeprom = parent->append<Node::Debugger::Memory>("Cartridge EEPROM");
    memory.eeprom->setSize(self.eeprom.size);
    memory.eeprom->setRead([&](u32 address) -> u8 {
      return self.eeprom.data[address];
    });
    memory.eeprom->setWrite([&](u32 address, u8 data) -> void {
      self.eeprom.data[address] = data;
    });
  }

  properties.ports = parent->append<Node::Debugger::Properties>("Cartridge I/O");
  properties.ports->setQuery([&] { return ports(); });
}

auto Cartridge::Debugger::unload(Node::Object parent) -> void {
  parent->remove(properties.ports);
  parent->remove(memory.rom);
  if(self.has.sram) parent->remove(memory.ram);
  if(self.has.eeprom) parent->remove(memory.eeprom);
  properties.ports.reset();
  memory.rom.reset();
  if(self.has.sram) memory.ram.reset();
  if(self.has.eeprom) memory.eeprom.reset();
}

auto Cartridge::Debugger::ports() -> string {
  string output;

  output.append("ROM Bank 0: ", hex(self.io.romBank0, 4L), "\n");
  output.append("ROM Bank 1: ", hex(self.io.romBank1, 4L), "\n");
  output.append("ROM Bank Linear: ", hex(self.io.romBank2, 4L), "\n");
  output.append("SRAM Bank: ", hex(self.io.sramBank, 4L), "\n");

  // TODO: RTC, GPO

  output.append("SRAM Bank Mode: ", self.io.flashEnable ? "Flash" : "SRAM", "\n");
  if(self.has.flash) {
    output.append("Flash Mode: ");
    u32 iAdded = 0;
    if(self.flash.idmode) output.append((iAdded++ > 0) ? ", " : "", "ID");
    if(self.flash.programmode) output.append((iAdded++ > 0) ? ", " : "", "Program");
    if(self.flash.fastmode) output.append((iAdded++ > 0) ? ", " : "", "Fast");
    if(self.flash.erasemode) output.append((iAdded++ > 0) ? ", " : "", "Erase");
    output.append("\n");
  }

  return output;
}
