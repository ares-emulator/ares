auto Cartridge::readIO(uint cycle, uint16 address, uint8 data) -> uint8 {
  if(address == 0xff50 && cycle == 2) return data;
  if(bootromEnable) {
    if(address >= 0x0000 && address <= 0x00ff && cycle == 2) return system.bootROM.read(address);
    if(address >= 0x0200 && address <= 0x08ff && cycle == 2 && Model::GameBoyColor()) return system.bootROM.read(address - 0x100);
  }

  if(address >= 0x0000 && address <= 0x7fff && cycle == 2) {
    return mapper->read(address);
  }

  if(address >= 0xa000 && address <= 0xbfff && cycle == 2) {
    return mapper->read(address);
  }

  return data;
}

auto Cartridge::writeIO(uint cycle, uint16 address, uint8 data) -> void {
  if(bootromEnable && address == 0xff50 && cycle == 2) return void(bootromEnable = false);

  if(address >= 0x0000 && address <= 0x7fff && cycle == 2) {
    mapper->write(address, data);
    return;
  }

  if(address >= 0xa000 && address <= 0xbfff && cycle == 2) {
    mapper->write(address, data);
    return;
  }
}
