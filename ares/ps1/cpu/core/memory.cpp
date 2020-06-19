auto CPU::readAddress(u32 address) -> maybe<u32> {
  return nothing;
}

auto CPU::writeAddress(u32 address) -> maybe<u32> {
  return nothing;
}

auto CPU::readByte(u32 address) -> maybe<u8> {
  if(auto physical = readAddress(address)) return bus.readByte(physical());
  return nothing;
}

auto CPU::readHalf(u32 address) -> maybe<u16> {
  if(auto physical = readAddress(address)) return bus.readHalf(physical());
  return nothing;
}

auto CPU::readWord(u32 address) -> maybe<u32> {
  if(auto physical = readAddress(address)) return bus.readWord(physical());
  return nothing;
}

auto CPU::writeByte(u32 address, u8 data) -> bool {
  if(auto physical = writeAddress(address)) return bus.writeByte(physical(), data), true;
  return false;
}

auto CPU::writeHalf(u32 address, u16 data) -> bool {
  if(auto physical = writeAddress(address)) return bus.writeHalf(physical(), data), true;
  return false;
}

auto CPU::writeWord(u32 address, u32 data) -> bool {
  if(auto physical = writeAddress(address)) return bus.writeWord(physical(), data), true;
  return false;
}
