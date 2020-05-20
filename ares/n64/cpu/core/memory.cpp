auto CPU::readAddress(u32 address) -> maybe<u32> {
  switch(context.segment[address >> 29]) {
  case Context::Segment::Invalid:
    exception.addressLoad();
    return nothing;
  case Context::Segment::Mapped:
    if(auto lookup = tlb.load(address)) return lookup();
    tlb.exception(address);
    return nothing;
  case Context::Segment::Cached:
  case Context::Segment::Uncached:
  default:
    return address;
  }
}

auto CPU::writeAddress(u32 address) -> maybe<u32> {
  switch(context.segment[address >> 29]) {
  case Context::Segment::Invalid:
    exception.addressStore();
    return nothing;
  case Context::Segment::Mapped:
    if(auto lookup = tlb.store(address)) return lookup();
    tlb.exception(address);
    return nothing;
  case Context::Segment::Cached:
  case Context::Segment::Uncached:
  default:
    return address;
  }
}

auto CPU::readByte(u32 address) -> maybe<u32> {
  if(auto physical = readAddress(address)) return bus.readByte(physical());
  return nothing;
}

auto CPU::readHalf(u32 address) -> maybe<u32> {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & 1)) return exception.addressLoad(), nothing;
  }
  if(auto physical = readAddress(address)) return bus.readHalf(physical());
  return nothing;
}

auto CPU::readWord(u32 address) -> maybe<u32> {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & 3)) return exception.addressLoad(), nothing;
  }
  if(auto physical = readAddress(address)) return bus.readWord(physical());
  return nothing;
}

auto CPU::readDouble(u32 address) -> maybe<u64> {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & 7)) return exception.addressLoad(), nothing;
  }
  if(auto physical = readAddress(address)) return bus.readDouble(physical());
  return nothing;
}

auto CPU::writeByte(u32 address, u8 data) -> bool {
  if(auto physical = writeAddress(address)) return bus.writeByte(physical(), data), true;
  return false;
}

auto CPU::writeHalf(u32 address, u16 data) -> bool {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & 1)) return exception.addressStore(), false;
  }
  if(auto physical = writeAddress(address)) return bus.writeHalf(physical(), data), true;
  return false;
}

auto CPU::writeWord(u32 address, u32 data) -> bool {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & 3)) return exception.addressStore(), false;
  }
  if(auto physical = writeAddress(address)) return bus.writeWord(physical(), data), true;
  return false;
}

auto CPU::writeDouble(u32 address, u64 data) -> bool {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & 7)) return exception.addressStore(), false;
  }
  if(auto physical = writeAddress(address)) return bus.writeDouble(physical(), data), true;
  return false;
}
