auto CPU::readByte(u64 address) -> maybe<u32> {
  address = tlbLookup(address);
  return bus.readByte(address);
}

auto CPU::readHalf(u64 address) -> maybe<u32> {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 1) return exception.addressLoad(), nothing;
  }
  address = tlbLookup(address);
  return bus.readHalf(address);
}

auto CPU::readWord(u64 address) -> maybe<u32> {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 3) return exception.addressLoad(), nothing;
  }
  address = tlbLookup(address);
  return bus.readWord(address);
}

auto CPU::readDouble(u64 address) -> maybe<u64> {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 7) return exception.addressLoad(), nothing;
  }
  address = tlbLookup(address);
  return bus.readDouble(address);
}

auto CPU::writeByte(u64 address, u8 data) -> void {
  address = tlbLookup(address);
  return bus.writeByte(address, data);
}

auto CPU::writeHalf(u64 address, u16 data) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 1) return exception.addressStore();
  }
  address = tlbLookup(address);
  return bus.writeHalf(address, data);
}

auto CPU::writeWord(u64 address, u32 data) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 3) return exception.addressStore();
  }
  address = tlbLookup(address);
  return bus.writeWord(address, data);
}

auto CPU::writeDouble(u64 address, u64 data) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 7) return exception.addressStore();
  }
  address = tlbLookup(address);
  return bus.writeDouble(address, data);
}
