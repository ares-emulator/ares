auto Cartridge::read(u32 cycle, n16 address, n8 data) -> n8 {
  if(address == 0xff50 && cycle == 2) {
    return data;
  }

  if(bootromEnable) {
    if(address >= 0x0000 && address <= 0x00ff && cycle == 2) {
      return system.bootROM.read(address);
    }

    if(address >= 0x0200 && address <= 0x08ff && cycle == 2 && Model::GameBoyColor()) {
      return system.bootROM.read(address - 0x100);
    }
  }

  if(address >= 0x0000 && address <= 0x7fff && cycle == 2) {
    return board->read(address, data);
  }

  if(address >= 0xa000 && address <= 0xbfff && cycle == 2) {
    return board->read(address, data);
  }

  return data;
}

auto Cartridge::write(u32 cycle, n16 address, n8 data) -> void {
  if(bootromEnable && address == 0xff50 && cycle == 2) {
    bootromEnable = false;  //does the written value matter?
    return;
  }

  if(address >= 0x0000 && address <= 0x7fff && cycle == 2) {
    return board->write(address, data);
  }

  if(address >= 0xa000 && address <= 0xbfff && cycle == 2) {
    return board->write(address, data);
  }
}
