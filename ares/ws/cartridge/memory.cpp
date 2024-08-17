//20000-fffff
auto Cartridge::readROM(n20 address) -> n8 {
  int busbyte = ((address & 1) && cpu.width(address) == Word) ? 1 : 0;
  if(!rom) return openbus.byte(busbyte);

  n8 value;
  n32 offset;
  switch(address.bit(16,19)) {
  case 2:  offset = io.romBank0 << 16 | (n16)address; break;  //20000-2ffff
  case 3:  offset = io.romBank1 << 16 | (n16)address; break;  //30000-3ffff
  default: offset = io.romBank2 << 20 | (n20)address; break;  //40000-fffff
  }

  if(has.flash && (flash.idmode || flash.fastmode)) {
    n16 data = flash.read(offset >> 1, true);
    if(address & 0x1) value = data >> 8;
    else value = data;
  } else value = rom.read(offset);

  openbus.byte(busbyte) = value;
  return value;
}

//10000-1ffff
auto Cartridge::readRAM(n20 address) -> n8 {
  n32 offset = io.sramBank << 16 | (n16)address;
  if(io.flashEnable) {
    if(!has.flash) return openbus;
    n8 value = flash.read(offset, false);
    openbus.byte(0) = value;
    return value;
  } else {
    if(!has.sram) return openbus;
    n8 value = ram.read(offset);
    openbus.byte(0) = value;
    return value;
  }
}

auto Cartridge::writeRAM(n20 address, n8 data) -> void {
  openbus.byte(0) = data;

  n32 offset = io.sramBank << 16 | (n16)address;
  if(io.flashEnable) {
    if(!has.flash) return;
    flash.write(offset, data);
  } else {
    if(!has.sram) return;
    ram.write(offset, data);
  }
}
