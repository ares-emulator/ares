inline auto Bus::mmio(u32 address) -> Memory::Interface& {
  if(address <= 0x007f'ffff)                 return cpu.ram;
  if(address >= 0x1fc0'0000)                 return bios;
  if((address & 0xffff'fc00) == 0x1f80'0000) return cpu.scratchpad;
  if((address & 0xffff'fff0) >= 0x1f80'1000 && (address & 0xffff'fff0) <= 0x1f80'1023) return memory;
  if((address & 0xffff'fff0) >= 0x1f80'1060 && (address & 0xffff'fff0) <= 0x1f80'1063) return memory;

  if(cpu.active()) cpu.ioSynchronize();

  if((address & 0xffff'fff0) >= 0x1f80'1070 && (address & 0xffff'fff0) <= 0x1f80'1074) return interrupt;
  if((address & 0xff80'0000) == 0x1f00'0000) return expansion1;
  if((address & 0xffff'fff0) >= 0x1f80'1040 && (address & 0xffff'fff0) <= 0x1f80'105f) return peripheral;
  if((address & 0xffff'fff0) >= 0x1f80'1080 && (address & 0xffff'fff0) <= 0x1f80'10ff) return dma;
  if((address & 0xffff'fff0) >= 0x1f80'1100 && (address & 0xffff'fff0) <= 0x1f80'112f) return timer;
  if((address & 0xffff'fff0) == 0x1f80'1800) return disc;
  if((address & 0xffff'fff0) == 0x1f80'1810) return gpu;
  if((address & 0xffff'fff0) == 0x1f80'1820) return mdec;
  if((address & 0xffff'fc00) == 0x1f80'1c00) return spu;
  if((address & 0xffff'f000) == 0x1f80'2000) return expansion2;
  if((address & 0xffff'0000) == 0x1fa0'0000) return expansion3;

  debug(unusual, "Bus::mmio(", hex(address, 8L), ")");
  return unmapped;
}

template<bool isWrite, bool isDMA>
auto Bus::calcAccessTime(u32 address, u32 bytesCount) -> u32 const {
  address &= 0x1fff'ffff;
  u32 words = (bytesCount > 0) ? ((bytesCount + 3) / 4) : 1;

  if(!isDMA && cpu.active()) cpu.waitDMA();

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
      // It takes 1 cycle to write to the W-Buffer
      // TODO: W-Buffer is emptied at 1 entry per 4 cycles;
      // TODO: Each additional write to the same 1K page takes two additional cycles
      // TODO: If w-buffer is full, we stall until the next entry is free
      // TODO: If we read from an address that is stored in the W-buffer, we stall until the write completes
      // TODO: Do not halt for DMA if we are writing to the W-Buffer, only if we hit the bus
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
  if((address & 0xffff'f000) == 0x1f80'2000) return memory.exp2.calcAccessTime<isWrite, isDMA>(bytesCount);
  if((address & 0xffc0'0000) == 0x1fa0'0000) return memory.exp3.calcAccessTime<isWrite, isDMA>(bytesCount);

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
