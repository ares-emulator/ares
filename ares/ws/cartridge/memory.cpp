//20000-fffff
auto Cartridge::readROM(n20 address) -> n8 {
  if(!rom) return 0x00;
  n32 offset;
  switch(address.bit(16,19)) {
  case 2:  offset = io.romBank0 << 16 | (n16)address; break;  //20000-2ffff
  case 3:  offset = io.romBank1 << 16 | (n16)address; break;  //30000-3ffff
  default: offset = io.romBank2 << 20 | (n20)address; break;  //40000-fffff
  }
  if(has.flash && (flash.idmode || flash.fastmode)) {
    n16 data = flash.read(offset >> 1, true);
    if(address & 0x1) return data >> 8;
    return data;
  }
  return rom.read(offset);
}

auto Cartridge::writeROM(n20 address, n8 data) -> void {
}

//10000-1ffff
auto Cartridge::readRAM(n20 address) -> n8 {
  n32 offset = io.sramBank << 16 | (n16)address;
  if(io.flashEnable) {
    if(!has.flash) return 0x00;
    return flash.read(offset, false);
  } else {
    if(!has.sram) return 0x00;
    return ram.read(offset);
  }
}

auto Cartridge::writeRAM(n20 address, n8 data) -> void {
  n32 offset = io.sramBank << 16 | (n16)address;
  if(io.flashEnable) {
    if(!has.flash) return;
    flash.write(offset, data);
  } else {
    if(!has.sram) return;
    ram.write(offset, data);
  }
}
