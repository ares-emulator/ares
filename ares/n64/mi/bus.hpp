template<u32 Size>
inline auto MI::readRdram(u32 address, RBusDevice device, Thread& thread) -> u64 {
  if(address <= 0x03ef'ffff) {
    if(unlikely(io.ebusTestMode) && device == RBusDevice::VR4300_UNCACHED)
      return rdram.ram.ebusRead<Size>(address);
    return rdram.ram.read<Size>(address, device);
  }

  u64 data = rdram.read<Size>(address, thread);
  if(device == RBusDevice::VR4300_UNCACHED && !io.rdramRegisterSelect && (address & 4)) data = 0;
  return data;
}

template<u32 Size>
inline auto MI::writeRdramRepeat(u32 address, u64 value) -> void {
  u8 length = io.repeatLength + 1;

  if constexpr(Size == Byte) {
    value = value & (0xFFFFFFFF >> (24 - (address & 3) * 8));
    value = (u32)((value << 24) | (value >> 8));
  } else if constexpr(Size == Half) {
    value = value & (0xFFFFFFFF >> (16 - (address & 2) * 8));
    value = (u32)((value << 16) | (value >> 16));
  }
  if constexpr(Size != Dual)
    value = (value << 32) | (u32)value;

  const u32 end = min((address & ~7) + length, rdram.ram.size);
  if(end <= address) return;
  length = end - address;

  if(address & 1) {
    rdram.ram.write<Byte>(address, value >> 56, RBusDevice::VR4300_UNCACHED);
    value = (value << 8) | (value >> 56);
    address = (address & ~0x7FF) | ((address + 1) & 0x7FF);
    length -= 1;
  }
  if((address & 2) && length >= 2) {
    rdram.ram.write<Half>(address, value >> 48, RBusDevice::VR4300_UNCACHED);
    value = (value << 16) | (value >> 48);
    address = (address & ~0x7FF) | ((address + 2) & 0x7FF);
    length -= 2;
  }
  if((address & 4) && length >= 4) {
    rdram.ram.write<Word>(address, value >> 32, RBusDevice::VR4300_UNCACHED);
    value = (value << 32) | (value >> 32);
    address = (address & ~0x7FF) | ((address + 4) & 0x7FF);
    length -= 4;
  }

  while(length >= 8) {
    rdram.ram.write<Dual>(address, value, RBusDevice::VR4300_UNCACHED);
    address = (address & ~0x7FF) | ((address + 8) & 0x7FF);
    length -= 8;
  }
  if(length >= 4) {
    rdram.ram.write<Word>(address, value >> 32, RBusDevice::VR4300_UNCACHED);
    value <<= 32;
    address += 4;
    length -= 4;
  }
  if(length >= 2) {
    rdram.ram.write<Half>(address, value >> 48, RBusDevice::VR4300_UNCACHED);
    value <<= 16;
    address += 2;
    length -= 2;
  }
  if(length == 1)
    rdram.ram.write<Byte>(address, value >> 56, RBusDevice::VR4300_UNCACHED);
}

template<u32 Size>
inline auto MI::writeRdram(u32 address, u64 value, RBusDevice device, Thread& thread) -> void {
  n1 repeat = 0;
  if(device == RBusDevice::VR4300_UNCACHED && unlikely(io.repeatMode)) {
    io.repeatMode = 0;
    repeat = 1;
  }

  if(address <= 0x03ef'ffff) {
    if(repeat) return writeRdramRepeat<Size>(address, value);
    if(unlikely(io.ebusTestMode) && device == RBusDevice::VR4300_UNCACHED)
      return rdram.ram.ebusWrite<Size>(address, value);
    return rdram.ram.write<Size>(address, value, device);
  }

  if(repeat) {
    u32 word = value;
    if constexpr(Size == Byte) word = (u32)value << (24 - 8 * (address & 3));
    if constexpr(Size == Half) word = (u32)value << (16 - 8 * (address & 2));
    if constexpr(Size == Dual) word = value >> 32;
    return rdram.writeWord(address, word, thread, io.repeatLength + 1);
  }
  rdram.write<Size>(address, value, thread);
}

template<u32 Size>
inline auto MI::readRdramBurst(u32 address, u32* data, RBusDevice device, Thread& thread) -> void {
  if(unlikely(io.ebusTestMode)) return ebusFreeze();

  if(address <= 0x03ef'ffff) {
    rdram.ram.readBurst<Size>(address, data, device);
    return;
  }

  data[0] = rdram.readWord(address | 0x0, thread);
  data[1] = 0;
  data[2] = 0;
  data[3] = 0;
  if constexpr(Size == ICache) {
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
  }
}

template<u32 Size>
inline auto MI::writeRdramBurst(u32 address, u32* data, RBusDevice device, Thread& thread) -> void {
  if(unlikely(io.ebusTestMode)) return ebusFreeze();

  if(address <= 0x03ef'ffff) {
    rdram.ram.writeBurst<Size>(address, data, device);
    return;
  }

  rdram.writeWord(address | 0x0, data[0], thread);
}

inline auto MI::ebusFreeze() -> void {
  debug(unusual, "[MI] cached RDRAM access while ebus mode is enabled; console hangs");
  cpu.scc.sysadFrozen = true;
}
