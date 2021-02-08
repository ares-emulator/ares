auto SufamiTurboCartridge::readROM(n24 address, n8 data) -> n8 {
  if(!rom) return data;
  return rom.read(address & rom.size() - 1, data);
}

auto SufamiTurboCartridge::writeROM(n24 address, n8 data) -> void {
  if(!rom) return;
  return rom.write(address & rom.size() - 1, data);
}

auto SufamiTurboCartridge::readRAM(n24 address, n8 data) -> n8 {
  if(!ram) return data;
  return ram.read(address & ram.size() - 1, data);
}

auto SufamiTurboCartridge::writeRAM(n24 address, n8 data) -> void {
  if(!ram) return;
  return ram.write(address & ram.size() - 1, data);
}
