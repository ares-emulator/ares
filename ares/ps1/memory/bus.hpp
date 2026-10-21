inline auto Bus::mmio(u32 address) -> Memory::Interface& {
  if(address <= 0x007f'ffff)                           return cpu.ram;
  if(address >= 0x1fc0'0000)                           return bios;
  if((address & 0xffff'fc00) == 0x1f80'0000)           return cpu.scratchpad;
  if(address >= 0x1f80'1000 && address <= 0x1f80'1023) return memory;
  if(address >= 0x1f80'1060 && address <= 0x1f80'1063) return memory;

  if(cpu.active()) cpu.ioSynchronize();

  if(address >= 0x1f80'1070 && address <= 0x1f80'1077)                                 return interrupt;
  if((address & ~memory.exp1Mask) == memory.exp1Address) {
    if(expansion.connected()) return expansion;
    return expansion1;
  }
  if((address & 0xffff'fff0) >= 0x1f80'1040 && (address & 0xffff'fff0) <= 0x1f80'105f) return peripheral;
  if((address & 0xffff'fff0) >= 0x1f80'1080 && (address & 0xffff'fff0) <= 0x1f80'10ff) return dma;
  if((address & 0xffff'fff0) >= 0x1f80'1100 && (address & 0xffff'fff0) <= 0x1f80'113f) return timer;
  if(address >= 0x1f80'1800 && address <= 0x1f80'1803)                                 return disc;
  if(address >= 0x1f80'1810 && address <= 0x1f80'1817)                                 return gpu;
  if(address >= 0x1f80'1820 && address <= 0x1f80'1827) return mdec;
  if((address & 0xffff'fc00) == 0x1f80'1c00) return spu;
  if(memory.exp2Enabled && (address & ~memory.exp2Mask) == 0x1f80'2000) return expansion2;
  if((address & 0xffe0'0000) == 0x1fa0'0000) return expansion3;

  debug(unusual, "Bus::mmio(", hex(address, 8L), ")");
  return unmapped;
}

inline auto Bus::busConfiguration(u32 address) const -> BusConfiguration {
  address &= 0x1fff'ffff;

  auto fromPort = [](const MemoryControl::MemPort& port) -> BusConfiguration {
    BusConfiguration result;
    result.width = port.dataWidth ? Half : Byte;
    result.autoIncrement = port.autoIncrement;
    return result;
  };

  if(address <= 0x007f'ffff) return {};
  if(address >= 0x1fc0'0000) return fromPort(memory.bios);
  if((address & 0xffff'fc00) == 0x1f80'0000) return {};
  if(address >= 0x1f80'1000 && address <= 0x1f80'1023) return {};
  if(address >= 0x1f80'1060 && address <= 0x1f80'1063) return {};
  if(address >= 0x1f80'1070 && address <= 0x1f80'1077) return {};
  if((address & ~memory.exp1Mask) == memory.exp1Address) return fromPort(memory.exp1);
  if((address & 0xffff'fff0) >= 0x1f80'1040 && (address & 0xffff'fff0) <= 0x1f80'105f) return {};
  if((address & 0xffff'fff0) >= 0x1f80'1080 && (address & 0xffff'fff0) <= 0x1f80'10ff) return {};
  if((address & 0xffff'fff0) >= 0x1f80'1100 && (address & 0xffff'fff0) <= 0x1f80'113f) return {};
  if(address >= 0x1f80'1800 && address <= 0x1f80'1803) return fromPort(memory.cdrom);
  if(address >= 0x1f80'1810 && address <= 0x1f80'1827) return {};
  if((address & 0xffff'fc00) == 0x1f80'1c00) {
    auto configuration = fromPort(memory.spu);
    configuration.width = Half;
    return configuration;
  }
  if(memory.exp2Enabled && (address & ~memory.exp2Mask) == 0x1f80'2000) return fromPort(memory.exp2);
  if((address & 0xffe0'0000) == 0x1fa0'0000) return fromPort(memory.exp3);
  return {};
}

template<bool isWrite, bool isDMA>
auto Bus::calcAccessTime(u32 address, u32 bytesCount) -> u32 const {
  address &= 0x1fff'ffff;
  u32 words = (bytesCount > 0) ? ((bytesCount + 3) / 4) : 1;

  if(likely(address <= 0x007f'ffff)) {
    if constexpr(isDMA) {
      // Hyper-Page DMA mode for DRAM: ~1 cycle per 32-bit word
      constexpr u32 wordsPerRow  = 16;
      constexpr u32 rowPenalty   = 1; // 1 extra cycle per row
      u32 rows = (words + wordsPerRow - 1) / wordsPerRow;
      u32 total = words + rows * rowPenalty;
      return total;
    }

    if constexpr(isWrite) {
      return 4 * words;
    }

    return 3 + words;
  }

  if(address >= 0x1fc0'0000) return memory.bios.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & ~memory.exp1Mask) == memory.exp1Address) return memory.exp1.calcAccessTime<isWrite, isDMA>(bytesCount);
  u32 internalTime = isDMA ? 1 : internalRegisterAccessTime;
  u32 devicePortTime = isDMA ? 1 : devicePortAccessTime;
  if(address >= 0x1f80'1070 && address <= 0x1f80'1077) return internalTime * words;
  if((address & 0xffff'fff0) >= 0x1f80'1100 && (address & 0xffff'fff0) <= 0x1f80'113f) return internalTime * words;
  if(address >= 0x1f80'1800 && address <= 0x1f80'1803) return memory.cdrom.calcAccessTime<isWrite, isDMA>(bytesCount);
  if(address >= 0x1f80'1810 && address <= 0x1f80'1817) return devicePortTime * words;
  if(address >= 0x1f80'1820 && address <= 0x1f80'1827) return devicePortTime * words;
  if((address & 0xffff'fc00) == 0x1f80'1C00) return memory.spu.calcAccessTime<isWrite, isDMA>(bytesCount);
  if(memory.exp2Enabled && (address & ~memory.exp2Mask) == 0x1f80'2000) return memory.exp2.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & 0xffe0'0000) == 0x1fa0'0000) return memory.exp3.calcAccessTime<isWrite, isDMA>(bytesCount);

  //debug(unusual, "Bus::calcAccessTime(", hex(address, 8L), ", ", hex(bytesCount, 8L), ")");
  return devicePortTime * words;
}

template<u32 Size>
inline auto Bus::read(u32 address) -> u32 {
  address &= 0x1fff'ffff;

  auto readWordBus = [&]() -> u32 {
    auto aligned = address & ~3;
    return mmio(aligned).readCycle({aligned, 0, Word, 0b1111}).data >> 8 * (address & 3);
  };

  auto readHalfBus = [&]() -> u32 {
    auto aligned = address & ~1;
    return mmio(aligned).readCycle({aligned, 0, Half, 0b11}).data >> 8 * (address & 1);
  };

  auto readByteNative = [&](u32 offset) -> u32 {
    auto target = address + offset;
    return mmio(target).readCycle({target, 0, Byte, 0b1}).data;
  };

  auto configuration = busConfiguration(address);
  u32 busWidth = configuration.width;
  auto busOffset = [&](u32 index) -> u32 {
    return configuration.autoIncrement ? busWidth * index : 0;
  };

  auto reads32BitBus = [&]() -> bool {
    if(address >= 0x1f80'1000 && address <= 0x1f80'1023) return true;
    if(address >= 0x1f80'1060 && address <= 0x1f80'1063) return true;
    if(address >= 0x1f80'1070 && address <= 0x1f80'1077) return true;

    u32 timer = address & 0x1fff'ffcc;
    if(timer >= 0x1f80'1100 && timer <= 0x1f80'1108) return true;

    if((address & 0x1fff'ff8c) == 0x1f80'1080) return true;
    if((address & 0x1fff'ff8c) == 0x1f80'1088) return true;
    if((address & 0x1fff'ff8c) == 0x1f80'108c) return true;
    if((address & ~3) == 0x1f80'10f0) return true;
    if((address & ~3) == 0x1f80'10f4) return true;
    if(address >= 0x1f80'1810 && address <= 0x1f80'1817) return true;
    if(address >= 0x1f80'1820 && address <= 0x1f80'1827) return true;

    return false;
  };

  auto reads16BitBus = [&]() -> bool {
    return address >= 0x1f80'1040 && address <= 0x1f80'105f;
  };

  if constexpr(Size == Byte) {
    if(busWidth == Half) {
      auto aligned = address & ~1;
      u32 data = mmio(aligned).readCycle({aligned, 0, Half, 0b11}).data;
      if(address & 2) readLatch = ((u32)readLatch & 0x0000'ffff) | data << 16;
      else            readLatch = ((u32)readLatch & 0xffff'0000) | data <<  0;
      return data >> 8 * (address & 1);
    }
    if(busWidth == Byte) return readByteNative(0);
    if(reads32BitBus()) return readWordBus();
    if(reads16BitBus()) return readHalfBus();
  }

  if constexpr(Size == Half) {
    if(busWidth == Byte) {
      return readByteNative(0) | readByteNative(busOffset(1)) << 8;
    }
    if(busWidth == Half) {
      u32 data = mmio(address).readCycle({address, 0, Half, 0b11}).data;
      if(address & 2) readLatch = ((u32)readLatch & 0x0000'ffff) | data << 16;
      else            readLatch = ((u32)readLatch & 0xffff'0000) | data <<  0;
      return data;
    }
    if(reads32BitBus()) return readWordBus();
  }

  if constexpr(Size == Word) {
    if(busWidth == Byte) {
      return readByteNative(0) | readByteNative(busOffset(1)) << 8 | readByteNative(busOffset(2)) << 16 | readByteNative(busOffset(3)) << 24;
    }

    if(busWidth == Half) {
      u32 nextAddress = address + busOffset(1);
      u32 lower = mmio(address).readCycle({address, 0, Half, 0b11}).data;
      u32 upper = mmio(nextAddress).readCycle({nextAddress, 0, Half, 0b11}).data;
      readLatch = lower | upper << 16;
      return readLatch;
    }

    if(address >= 0x1f80'1040 && address <= 0x1f80'105c) {
      auto lowerAddress = address & ~1;
      auto upperAddress = lowerAddress + 2;
      auto lower = mmio(lowerAddress).readCycle({lowerAddress, 0, Half, 0b11}).data;
      auto upper = mmio(upperAddress).readCycle({upperAddress, 0, Half, 0b11}).data;
      return lower | upper << 16;
    }
  }

  return mmio(address).readCycle({address, 0, Size, (1u << Size) - 1}).data;
}

template<u32 Size>
inline auto Bus::write(u32 address, u32 data) -> void {
  address &= 0x1fff'ffff;

  auto writeWordBus = [&]() -> void {
    auto aligned = address & ~3;
    return mmio(aligned).writeCycle({aligned, data << 8 * (address & 3), Word, ((1u << Size) - 1) << (address & 3)});
  };

  auto writeHalfBus = [&]() -> void {
    auto aligned = address & ~1;
    return mmio(aligned).writeCycle({aligned, data << 8 * (address & 1), Half, 1u << (address & 1)});
  };

  auto writeByteNative = [&](u32 offset, u32 data) -> void {
    auto target = address + offset;
    return mmio(target).writeCycle({target, data, Byte, 0b1});
  };

  auto configuration = busConfiguration(address);
  u32 busWidth = configuration.width;
  auto busOffset = [&](u32 index) -> u32 {
    return configuration.autoIncrement ? busWidth * index : 0;
  };

  auto writes32BitBus = [&]() -> bool {
    if(address >= 0x1f80'1000 && address <= 0x1f80'1023) return true;
    if(address >= 0x1f80'1060 && address <= 0x1f80'1063) return true;
    if(address >= 0x1f80'1070 && address <= 0x1f80'1077) return true;

    u32 timer = address & 0x1fff'ffcc;
    if(timer >= 0x1f80'1100 && timer <= 0x1f80'1108) return true;

    if((address & 0x1fff'ff8c) == 0x1f80'1080) return true;
    if((address & 0x1fff'ff8c) == 0x1f80'1088) return true;
    if((address & 0x1fff'ff8c) == 0x1f80'108c) return true;
    if((address & ~3) == 0x1f80'10f0) return true;
    if((address & ~3) == 0x1f80'10f4) return true;
    if(address >= 0x1f80'1810 && address <= 0x1f80'1817) return true;
    if(address >= 0x1f80'1820 && address <= 0x1f80'1827) return true;

    return false;
  };

  auto writes16BitBus = [&]() -> bool {
    return address >= 0x1f80'1040 && address <= 0x1f80'105f;
  };

  auto ignoresWrite = [&]() -> bool {
    return (address >= 0x1f80'10f8 && address <= 0x1f80'10ff)
    || (address >= 0x1f80'1e80 && address <= 0x1f80'1fff);
  };

  if(ignoresWrite()) return;

  if constexpr(Size == Byte) {
    if(busWidth == Half) {
      return writeHalfBus();
    }
    if(busWidth == Byte) return writeByteNative(0, data);
    if(writes32BitBus()) return writeWordBus();
    if(writes16BitBus()) return writeHalfBus();
  }

  if constexpr(Size == Half) {
    if(busWidth == Byte) {
      writeByteNative(0, data >> 0);
      return writeByteNative(busOffset(1), data >> 8);
    }
    if(busWidth == Half) return mmio(address).writeCycle({address, data, Half, 0b11});
    if(writes32BitBus()) return writeWordBus();
  }

  if constexpr(Size == Word) {
    if(busWidth == Byte) {
      writeByteNative(0, data >>  0);
      writeByteNative(busOffset(1), data >>  8);
      writeByteNative(busOffset(2), data >> 16);
      return writeByteNative(busOffset(3), data >> 24);
    }

    if(busWidth == Half) {
      u32 nextAddress = address + busOffset(1);
      mmio(address).writeCycle({address, data >> 0, Half, 0b11});
      return mmio(nextAddress).writeCycle({nextAddress, data >> 16, Half, 0b11});
    }

    if(writes16BitBus()) {
      auto aligned = address & ~1;
      return mmio(aligned).writeCycle({aligned, data, Half, 0b11});
    }
  }

  return mmio(address).writeCycle({address, data, Size, (1u << Size) - 1});
}
