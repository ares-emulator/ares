auto RSP::readByte(u32 address) -> maybe<u32> {
  return dmem.readByte(address);
}

auto RSP::readHalf(u32 address) -> maybe<u32> {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 1) return nothing;
  }
  return dmem.readHalf(address);
}

auto RSP::readWord(u32 address) -> maybe<u32> {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 3) return nothing;
  }
  return dmem.readWord(address);
}

auto RSP::writeByte(u32 address, u8 data) -> void {
  return dmem.writeByte(address, data);
}

auto RSP::writeHalf(u32 address, u16 data) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 1) return;
  }
  return dmem.writeHalf(address, data);
}

auto RSP::writeWord(u32 address, u32 data) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(address & 3) return;
  }
  return dmem.writeWord(address, data);
}
