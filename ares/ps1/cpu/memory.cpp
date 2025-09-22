//read code from the bus
inline auto CPU::fetch(u32 address) -> u32 {
  switch(address >> 29) {
  //cached
  case 0:  //$00000000-$1fffffff  KUSEG
  case 4: {//$80000000-$9fffffff  KSEG0
    return icache.fetch(address);
  }

  //uncached
  case 5: {//$a0000000-$bfffffff  KSEG1
    if(likely(address <= 0xa07f'ffff)) {
      step(bus.calcAccessTime<false, false>(address, Word));
      auto data = ram.read<Word>(address);
      return data;
    }
    if(likely(address >= 0xbfc0'0000)) {
      step(bus.calcAccessTime<false, false>(address, Word));
      auto data = bios.read<Word>(address);
      return data;
    }
    if(likely(address >= 0xbf00'0000)) {
      //the CPU cannot execute out of the scratchpad or (most) MMIO register areas
      debug(unhandled, "CPU::fetch");
      return 0;
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busInstruction();
    }
    return 0;  //nop
  }

  //unmapped
  case 1:  //$20000000-$3fffffff  KUSEG
  case 2:  //$40000000-$5fffffff  KUSEG
  case 3:  //$60000000-$7fffffff  KUSEG
  case 6:  //$c0000000-$dfffffff  KSEG2
  case 7: {//$e0000000-$ffffffff  KSEG2
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busInstruction();
    }
    return 0;  //nop
  }

  }
  unreachable;
}

//peek at the next instruction, does not consume cycles
inline auto CPU::peek(u32 address) -> u32 {
  address &= 0x1fff'ffff;

  if(likely(address <= 0x007f'ffff)) {
    return ram.read<Word>(address);
  }

  if(likely(address >= 0x1fc0'0000)) {
    return bios.read<Word>(address);
  }

  debug(unimplemented, "CPU::peek ", hex(address));
  return 0;
}

//read data from the bus
template<u32 Size>
inline auto CPU::read(u32 address) -> u32 {
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(breakpoint.testData<Read, Size>(address)) return 0;  //nop
  }

  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) {
      if(unlikely(address & 1)) return exception.address<Read>(address), 0;  //nop
    }
    if constexpr(Size == Word) {
      if(unlikely(address & 3)) return exception.address<Read>(address), 0;  //nop
    }
  }

  if(unlikely(address >= 0xfffe'0000)) {
    return memory.read<Size>(address);
  }

  switch(address >> 29) {
  //cached
  case 0: {//KUSEG
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.tagTest == 1 && memory.cache.codeEnable == 1) {
        return icache.read(address);
      }
      if(memory.cache.tagTest == 0 && memory.cache.scratchpadEnable == 1) {
        return scratchpad.read<Size>(address);
      }
      return 0;  //nop
    }
    if(likely(address <= 0x007f'ffff)) {
      step(bus.calcAccessTime<false, false>(address, Size));
      auto data = ram.read<Size>(address);
      return data;
    }
    if(likely(address >= 0x1fc0'0000)) {
      auto data = memory.read<Size>(address);
      return data;
    }
    if(likely(address >= 0x1f00'0000)) {
      step(bus.calcAccessTime<false, false>(address, Size));
      address &= 0x1fff'ffff;
      auto& memory = bus.mmio(address);
      auto data = memory.read<Size>(address);
      return data;
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;  //nop
  }

  //cached
  case 4: {//KSEG0
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.tagTest == 1 && memory.cache.codeEnable == 1) {
        return icache.read(address);
      }
      if(memory.cache.tagTest == 0 && memory.cache.scratchpadEnable == 1) {
        return scratchpad.read<Size>(address);
      }
      return 0;  //nop
    }
    if(likely(address <= 0x807f'ffff)) {
      step(bus.calcAccessTime<false, false>(address, Size));
      auto data = ram.read<Size>(address);
      return data;
    }
    if(likely(address >= 0x9fc0'0000)) {
      auto data = memory.read<Size>(address);
      return data;
    }
    if(likely(address >= 0x9f00'0000)) {
      step(bus.calcAccessTime<false, false>(address, Size));
      address &= 0x1fff'ffff;
      auto& memory = bus.mmio(address);
      auto data = memory.read<Size>(address);
      return data;
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;  //nop
  }

  //uncached
  case 5: {//KSEG1
    if(likely(address <= 0xa07f'ffff)) {
      step(bus.calcAccessTime<false, false>(address, Size));
      auto data = ram.read<Size>(address);
      return data;
    }
    if(likely(address >= 0xbfc0'0000)) {
      step(bus.calcAccessTime<false, false>(address, Size));
      auto data = bios.read<Size>(address);
      return data;
    }
    if(likely(address >= 0xbf00'0000)) {
      step(bus.calcAccessTime<false, false>(address, Size));
      address &= 0x1fff'ffff;
      auto& memory = bus.mmio(address);
      auto data = memory.read<Size>(address);
      return data;
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;  //nop
  }

  //unmapped
  case 1:  //KUSEG
  case 2:  //KUSEG
  case 3:  //KUSEG
  case 6:  //KSEG2
  case 7: {//KSEG2
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;  //nop
  }

  }

  unreachable;
}

//write data to the bus
template<u32 Size>
inline auto CPU::write(u32 address, u32 data) -> void {
  //NOTE: CPU has a write fifo that prevents stalls until full
  //for now we disable write cycle timing to simulate an infinite write fifo
  //once we implement the real thing; then we can uncomment the step() calls
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(breakpoint.testData<Write, Size>(address)) return;
  }

  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) {
      if(unlikely(address & 1)) return exception.address<Write>(address);
    }
    if constexpr(Size == Word) {
      if(unlikely(address & 3)) return exception.address<Write>(address);
    }
  }

  if(unlikely(address >= 0xfffe'0000)) {
    return memory.write<Size>(address, data);
  }

  switch(address >> 29) {
  //cached
  case 0: {//KUSEG
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.tagTest == 1 && memory.cache.codeEnable == 1) {
        return icache.invalidate(address);
      }
      if(memory.cache.tagTest == 0 && memory.cache.scratchpadEnable == 1) {
        return scratchpad.write<Size>(address, data);
      }
      return;
    }
    if(likely(address <= 0x007f'ffff)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      return ram.write<Size>(address, data);
    }
    if(likely(address >= 0x1fc0'0000)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      return bios.write<Size>(address, data);
    }
    if(likely(address >= 0x1f00'0000)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      address &= 0x1fff'ffff;
      auto& memory = bus.mmio(address);
      return memory.write<Size>(address, data);
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return;
  }

  //cached
  case 4: {//KSEG0
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.tagTest == 1 && memory.cache.codeEnable == 1) {
        return icache.invalidate(address);
      }
      if(memory.cache.tagTest == 0 && memory.cache.scratchpadEnable == 1) {
        return scratchpad.write<Size>(address, data);
      }
      return;
    }
    if(likely(address <= 0x807f'ffff)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      return ram.write<Size>(address, data);
    }
    if(likely(address >= 0x9fc0'0000)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      return bios.write<Size>(address, data);
    }
    if(likely(address >= 0x9f00'0000)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      address &= 0x1fff'ffff;
      auto& memory = bus.mmio(address);
      return memory.write<Size>(address, data);
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return;
  }

  //uncached
  case 5: {//KSEG1
    if(likely(address <= 0xa07f'ffff)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      return ram.write<Size>(address, data);
    }
    if(likely(address >= 0xbfc0'0000)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      return bios.write<Size>(address, data);
    }
    if(likely(address >= 0xbf00'0000)) {
      step(bus.calcAccessTime<true, false>(address, Size));
      address &= 0x1fff'ffff;
      auto& memory = bus.mmio(address);
      return memory.write<Size>(address, data);
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return;
  }

  //unmapped
  case 1:  //KUSEG
  case 2:  //KUSEG
  case 3:  //KUSEG
  case 6:  //KSEG2
  case 7: {//KSEG2
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return;
  }

  }
}
