auto SC64::open(string location, bool readOnly_) -> bool {
  close();
  if(!location) return false;

  readOnly = readOnly_;
  auto mode = readOnly ? file::mode::read : file::mode::modify;
  if(!image.open(location, mode)) return false;
  if(image.size() == 0 || image.size() % 512) {
    image.close();
    return false;
  }

  buffer.allocate(8_KiB, 0);
  sectorCount = image.size() / 512;
  enabled = true;
  sdInitialized = false;
  sdBlockAddressed = false;
  sdClock50MHz = false;
  byteSwap = false;
  unlocked = false;
  registers = {};
  registers.identifier = 0x53437632;
  return true;
}

auto SC64::close() -> void {
  flush();
  image.close();
  buffer.reset();
  enabled = false;
  sdInitialized = false;
  sdBlockAddressed = false;
  sdClock50MHz = false;
  unlocked = false;
  byteSwap = false;
}

auto SC64::flush() -> void {
  if(image) image.flush();
}

auto SC64::piAddress(u32 address_, PIDeviceTiming timing) -> bool {
  if(!enabled || !timing.fasterThan({0, 0, 0})) return false;
  address_ &= ~1;

  if(unlocked && address_ >= 0x1ffe'0000 && address_ < 0x1ffe'2000) {
    target = Target::Buffer;
    offset = address_ - 0x1ffe'0000;
    return true;
  }

  if(unlocked && address_ >= 0x1fff'0000 && address_ < 0x1fff'001c) {
    target = Target::Registers;
    offset = address_ - 0x1fff'0000;
    return true;
  }

  if(address_ == 0x1fff'0010) {
    target = Target::Registers;
    offset = 0x10;
    return true;
  }

  return false;
}

auto SC64::piReadHalf(PIDeviceTiming) -> maybe<u16> {
  if(target == Target::Buffer) {
    if(offset >= buffer.size) return nothing;
    auto data = buffer.read<Half>(offset);
    offset += 2;
    return (u16)data;
  }

  if(target == Target::Registers) {
    auto address_ = offset & ~3;
    auto data = registerRead(address_);
    auto result = (offset & 2) ? (u16)data : (u16)(data >> 16);
    offset += 2;
    return result;
  }

  return nothing;
}

auto SC64::piWriteHalf(u16 data, PIDeviceTiming) -> void {
  if(target == Target::Buffer) {
    if(offset < buffer.size) buffer.write<Half>(offset, data);
    offset += 2;
    return;
  }

  if(target == Target::Registers) {
    auto address_ = offset & ~3;
    if(offset & 2) registerWrite(address_, pendingRegister | data);
    else pendingRegister = u32(data) << 16;
    offset += 2;
  }
}

auto SC64::registerRead(u32 address_) -> u32 {
  switch(address_) {
  case 0x00: return registers.scr;
  case 0x04: return registers.data0;
  case 0x08: return registers.data1;
  case 0x0c: return registers.identifier;
  case 0x14: return registers.irq;
  case 0x18: return registers.aux;
  default: return 0;
  }
}

auto SC64::registerWrite(u32 address_, u32 data) -> void {
  switch(address_) {
  case 0x00:
    registers.scr = (registers.scr & 0xff00'0000) | (data & 0x0000'01ff);
    if((data & 0xff) != 0) execute(data & 0xff);
    break;
  case 0x04: registers.data0 = data; break;
  case 0x08: registers.data1 = data; break;
  case 0x10:
    if(data == 0) unlocked = false;
    if(data == 0x5f55'4e4c) registers.aux = data;
    if(data == 0x4f43'4b5f && registers.aux == 0x5f55'4e4c) unlocked = true;
    if(data == 0xffff'ffff) unlocked = false;
    break;
  case 0x14:
    registers.scr &= ~(data & (1u << 27));
    registers.irq = data;
    break;
  case 0x18: registers.aux = data; break;
  }
}

// Match the status word returned by the SC64 SD controller.  The emulated
// image is always an inserted card; the remaining bits track the state
// maintained by the command interface.
auto SC64::sdStatus() const -> u32 {
  return (byteSwap ? 1u << 4 : 0u)
       | (sdClock50MHz ? 1u << 3 : 0u)
       | (sdBlockAddressed ? 1u << 2 : 0u)
       | (sdInitialized ? 1u << 1 : 0u)
       | (enabled ? 1u : 0u);
}

auto SC64::writeSdInfo(u32 address_) -> bool {
  if(!sdInitialized) return false;

  // The hardware exposes CSD followed by CID (16 bytes each).  The image is
  // modeled as an SDHC card, so synthesize the CSD capacity from its sector
  // count; the CID is intentionally stable but otherwise opaque.
  u8 info[32] = {};
  info[0] = 0x40;  // CSD structure version 2.0
  u64 csize = sectorCount >= 1024 ? (sectorCount / 1024) - 1 : 0;
  csize = min<u64>(csize, 0x3f'ffff);
  info[7] = (u8)(csize >> 16) & 0x3f;
  info[8] = (u8)(csize >> 8);
  info[9] = (u8)csize;

  if(u64(address_) + sizeof(info) <= 0x1ffe'2000 && address_ >= 0x1ffe'0000) {
    for(u32 index : range(sizeof(info))) buffer.write<Byte>(address_ - 0x1ffe'0000 + index, info[index]);
    return true;
  }
  if(u64(address_) + sizeof(info) <= 0x0080'0000) {
    for(u32 index : range(sizeof(info)))
      rdram.ram.write<Byte>(address_ + index, info[index], RBusDevice::ARES_DEBUGGER);
    return true;
  }
  return false;
}

auto SC64::complete() -> void {
  registers.scr &= ~(1u << 31);
}

auto SC64::error() -> void {
  registers.scr |= 1u << 30;
  complete();
}

auto SC64::execute(u8 command) -> void {
  registers.scr |= 1u << 31;
  registers.scr &= ~(1u << 30);

  switch(command) {
  case 'v':
    registers.data0 = registers.identifier;
    complete();
    return;
  case 'V':
    registers.data0 = 2 << 16;
    registers.data1 = 0;
    complete();
    return;
  case 'c':
    if(registers.data0 == 3) registers.data1 = 0;
    else if(registers.data0 == 6) registers.data1 = self.ram ? 3 : 0;
    else if(registers.data0 == 1) registers.data1 = 1;
    else registers.data1 = 0;
    complete();
    return;
  case 'C':
    complete();
    return;
  case 'i':
    {
    u32 result = 0;
    switch(registers.data1) {
    case 0:
      sdInitialized = false;
      sdBlockAddressed = false;
      sdClock50MHz = false;
      byteSwap = false;
      break;
    case 1:
      sdInitialized = true;
      sdBlockAddressed = true;
      sdClock50MHz = true;
      break;
    case 2: break;
    case 3:
      if(!writeSdInfo(registers.data0)) result = sdInitialized ? 4 : 2;
      break;
    case 4: byteSwap = true; break;
    case 5: byteSwap = false; break;
    default: result = 5; break; // SD_ERROR_INVALID_OPERATION
    }
    registers.data0 = result;
    registers.data1 = sdStatus();
    if(result) { error(); return; }
    complete();
    return;
    }
  case 'I':
    sdSector = registers.data0;
    complete();
    return;
  case 's':
    if(!sdInitialized || !transfer(false, registers.data0, registers.data1)) error();
    else { sdSector += registers.data1; complete(); }
    return;
  case 'S':
    if(readOnly || !sdInitialized || !transfer(true, registers.data0, registers.data1)) error();
    else { sdSector += registers.data1; flush(); complete(); }
    return;
  default:
    error();
    return;
  }
}

auto SC64::address(u32 value, u32 length) -> u8* {
  if(u64(value) + length <= 0x1ffe'2000 && value >= 0x1ffe'0000)
    return buffer.data + (value - 0x1ffe'0000);
  if(u64(value) + length <= u64(0x1000'0000) + self.rom.size && value >= 0x1000'0000)
    return self.rom.data + (value - 0x1000'0000);
  return nullptr;
}

auto SC64::setByteSwap(u8* data, u32 size) -> void {
  if(!byteSwap) return;
  for(u32 address_ = 0; address_ < size; address_ += 4)
    std::swap(data[address_ + 0], data[address_ + 3]), std::swap(data[address_ + 1], data[address_ + 2]);
}

auto SC64::transfer(bool write, u32 piAddress, u32 count) -> bool {
  if(!count || count > 0x7fffff || sdSector >= sectorCount || count > sectorCount - sdSector) return false;
  auto bytes = u64(count) * 512;
  auto target = address(piAddress, bytes);
  bool dram = piAddress < 0x0080'0000 && u64(piAddress) + bytes <= 0x0080'0000;
  bool bufferTarget = piAddress >= 0x1ffe'0000 && u64(piAddress) + bytes <= 0x1ffe'2000;
  if(!target && !dram) return false;

  u8 sector[512];
  for(u32 n : range(count)) {
    image.seek((u64(sdSector) + n) * 512);
    if(write) {
      for(u32 index : range(512)) {
        auto address_ = piAddress + n * 512 + index;
        sector[index] = dram ? rdram.ram.read<Byte>(address_, RBusDevice::ARES_DEBUGGER)
          : bufferTarget ? buffer.read<Byte>(address_ - 0x1ffe'0000) : target[n * 512 + index];
      }
      setByteSwap(sector, sizeof sector);
      for(auto byte : sector) image.write(byte);
    } else {
      for(auto& byte : sector) byte = image.read();
      setByteSwap(sector, sizeof sector);
      for(u32 index : range(512)) {
        auto address_ = piAddress + n * 512 + index;
        if(dram) rdram.ram.write<Byte>(address_, sector[index], RBusDevice::ARES_DEBUGGER);
        else if(bufferTarget) buffer.write<Byte>(address_ - 0x1ffe'0000, sector[index]);
        else target[n * 512 + index] = sector[index];
      }
    }
  }
  return true;
}
