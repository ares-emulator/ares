inline auto Bus::mmio(u32 address) -> Memory::Interface& {
  auto portMapped = [&](MemoryControl::MemPort& port, u32 base) -> bool {
    if(!port.configured) return true;
    u32 window = 1u << (u32)port.addrBits;
    if(address - base < window) return true;
    port.addrError = 1;
    return false;
  };

  if(auto access = memory.decodeRAM(address); access.type != MemoryControl::RAMAccess::NotRAM) {
    if(access.type == MemoryControl::RAMAccess::Mapped) return cpu.ram;
    return unmapped;
  }
  if(address >= 0x1fc0'0000) return portMapped(memory.bios, 0x1fc0'0000) ? (Memory::Interface&)bios : unmapped;
  if((address & 0xffff'fc00) == 0x1f80'0000) return cpu.scratchpad;
  if((address & 0xffff'fff0) >= 0x1f80'1000 && (address & 0xffff'fff0) <= 0x1f80'1023) return memory;
  if((address & 0xffff'fff0) >= 0x1f80'1060 && (address & 0xffff'fff0) <= 0x1f80'1063) return memory;

  if(cpu.active()) cpu.ioSynchronize();

  if((address & 0xffff'fff0) >= 0x1f80'1070 && (address & 0xffff'fff0) <= 0x1f80'1074) return interrupt;
  if((address & 0xff80'0000) == 0x1f00'0000) {
    return portMapped(memory.exp1, 0x1f00'0000) ? (Memory::Interface&)expansion1 : unmapped;
  }
  if((address & 0xffff'fff0) >= 0x1f80'1040 && (address & 0xffff'fff0) <= 0x1f80'105f) return peripheral;
  if((address & 0xffff'fff0) >= 0x1f80'1080 && (address & 0xffff'fff0) <= 0x1f80'10ff) return dma;
  if((address & 0xffff'fff0) >= 0x1f80'1100 && (address & 0xffff'fff0) <= 0x1f80'112f) return timer;
  if((address & 0xffff'fff0) == 0x1f80'1800) return disc;
  if((address & 0xffff'fff0) == 0x1f80'1810) return gpu;
  if((address & 0xffff'fff0) == 0x1f80'1820) return mdec;
  if((address & 0xffff'fc00) == 0x1f80'1c00) return spu;
  if((address & 0xffff'e000) == 0x1f80'2000) {
    return portMapped(memory.exp2, 0x1f80'2000) ? (Memory::Interface&)expansion2 : unmapped;
  }
  if((address & 0xffe0'0000) == 0x1fa0'0000) {
    return portMapped(memory.exp3, 0x1fa0'0000) ? (Memory::Interface&)expansion3 : unmapped;
  }

  debug(unusual, "Bus::mmio(", hex(address, 8L), ")");
  return unmapped;
}

template<bool isWrite, bool isDMA>
auto Bus::calcAccessTime(u32 address, u32 bytesCount) -> u32 const {
  address &= 0x1fff'ffff;
  u32 words = (bytesCount > 0) ? ((bytesCount + 3) / 4) : 1;

  if(auto access = memory.decodeRAM(address); access.type == MemoryControl::RAMAccess::Mapped) {
    if constexpr(isDMA) {
      // Hyper-Page DMA mode for DRAM: ~1 cycle per 32-bit word
      constexpr u32 wordsPerRow  = 16;
      constexpr u32 rowPenalty   = 1; // 1 extra cycle per row
      u32 rows = (words + wordsPerRow - 1) / wordsPerRow;
      u32 total = words + rows * rowPenalty;
      return total;
    }

    if constexpr(isWrite) {
      //CPU main-RAM stores use CPU::enqueueWriteBuffer(); this is the enqueue latency.
      return 1;
    }

    // initial penalty for 1-4 bytes and then an additional 1 cycle for each word after
    return 4 + words;
  }

  if(address >= 0x1fc0'0000) return memory.bios.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & 0xff80'0000) == 0x1f00'0000) return memory.exp1.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & 0xffff'fff0) == 0x1f80'1800) return memory.cdrom.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & 0xffff'fff0) == 0x1f80'1810) return 1 * words; // TODO: GPU access time depends on fifo states
  if((address & 0xffff'fff0) == 0x1f80'1820) return 1 * words; // TODO: MDEC access time depends on fifo/compression state
  if((address & 0xffff'fc00) == 0x1f80'1C00) return memory.spu.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & 0xffff'e000) == 0x1f80'2000) return memory.exp2.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & 0xffe0'0000) == 0x1fa0'0000) return memory.exp3.calcAccessTime<isWrite, isDMA>(bytesCount);

  //debug(unusual, "Bus::calcAccessTime(", hex(address, 8L), ", ", hex(bytesCount, 8L), ")");
  return 1  * words;
}

template<u32 Size>
inline auto Bus::read(u32 address) -> u32 {
  address &= 0x1fff'ffff;
  return mmio(address).read<Size>(address);
}

template<u32 Size>
inline auto Bus::write(u32 address, u32 data) -> void {
  address &= 0x1fff'ffff;
  return mmio(address).write<Size>(address, data);
}
