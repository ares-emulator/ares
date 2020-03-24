auto SufamiTurboCartridge::readROM(uint24 address, uint8 data) -> uint8 {
  if(!rom) return data;
  return rom.read(address & rom.size() - 1, data);
}

auto SufamiTurboCartridge::writeROM(uint24 address, uint8 data) -> void {
  if(!rom) return;
  return rom.write(address & rom.size() - 1, data);
}

auto SufamiTurboCartridge::readRAM(uint24 address, uint8 data) -> uint8 {
  if(!ram) return data;
  return ram.read(address & ram.size() - 1, data);
}

auto SufamiTurboCartridge::writeRAM(uint24 address, uint8 data) -> void {
  if(!ram) return;
  return ram.write(address & ram.size() - 1, data);
}
