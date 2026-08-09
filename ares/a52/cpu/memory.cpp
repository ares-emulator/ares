auto CPU::readBus(n16 address) -> n8 {
  if(auto result = platform->cheat(address)) return *result;

  if(address <= 0x3fff) return system.ram.read(address);
  if(address <= 0xbfff) return cartridge.read(address - 0x4000, io.openBus);
  if(address <= 0xc0ff) return gtia.read(address);
  if(address >= 0xd400 && address <= 0xd4ff) return antic.read(address);
  if(address >= 0xe800 && address <= 0xefff) return pokey.read(address);
  if(address >= 0xf800) return system.bios.read(address - 0xf800);
  return io.openBus;
}

auto CPU::peekBus(n16 address) const -> n8 {
  if(address <= 0x3fff) return system.ram.read(address);
  if(address <= 0xbfff) return cartridge.peek(address - 0x4000, io.openBus);
  if(address <= 0xc0ff) return gtia.peek(address);
  if(address >= 0xd400 && address <= 0xd4ff) return antic.peek(address);
  if(address >= 0xe800 && address <= 0xefff) return pokey.peek(address);
  if(address >= 0xf800) return system.bios.read(address - 0xf800);
  return io.openBus;
}

auto CPU::writeBus(n16 address, n8 data) -> void {
  if(address <= 0x3fff) return system.ram.write(address, data);
  if(address <= 0xbfff) return (void)cartridge.write(address - 0x4000, data);
  if(address <= 0xc0ff) return gtia.write(address, data);
  if(address >= 0xd400 && address <= 0xd4ff) return antic.write(address, data);
  if(address >= 0xe800 && address <= 0xefff) return pokey.write(address, data);
}

auto CPU::readDebugger(n16 address) -> n8 {
  return peekBus(address);
}
