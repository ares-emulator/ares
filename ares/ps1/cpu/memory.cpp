//read code from the bus
inline auto CPU::fetch(u32 address) -> u32 {
  if(likely((address & 0x7fff'ffff) <= 0x007f'ffff)) {
    return icache.fetch(address);
  }

  auto physicalAddress = address & 0x1fff'ffff;
  if((physicalAddress & 0xffff'fc00) == 0x1f80'0000
  || (physicalAddress & 0xffff'fff0) == 0x1f80'1070
  || (physicalAddress & 0xffff'fff0) == 0x1f80'1820
  ) {
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busInstruction();
    }
    return 0;
  }

  switch(address >> 29) {
  //cached
  case 0:  //$00000000-$1fffffff  KUSEG
  case 4: {//$80000000-$9fffffff  KSEG0
    return icache.fetch(address);
  }

  //uncached
  case 5: {//$a0000000-$bfffffff  KSEG1
    if(likely(address <= 0xa07f'ffff)) {
      beginBusTransaction(address, Word);
      auto data = ram.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0xbfc0'0000)) {
      beginBusTransaction(address, Word);
      auto data = bios.read<Word>(address);
      endBusTransaction();
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
inline auto CPU::readBus(u32 address) -> u32 {
  if(likely(!scc.status.cache.isolate && (address & 0x7fff'ffff) <= 0x007f'ffff)) {
    beginBusTransaction(address, Size);
    auto data = ram.read<Size>(address);
    endBusTransaction();
    return data;
  }

  if(unlikely(address >= 0xfffe'0000)) {
    return memory.read<Size>(address);
  }

  switch(address >> 29) {
  //cached
  case 0: {//KUSEG
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.codeEnable == 1) {
        return icache.read(address);
      }
      if(memory.cache.scratchpadEnable == 1) {
        return scratchpad.read<Size>(address);
      }
      return 0;  //nop
    }
    if(likely(address <= 0x007f'ffff)) {
      beginBusTransaction(address, Size);
      auto data = ram.read<Size>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x1fc0'0000)) {
      beginBusTransaction(address, Size);
      auto data = bios.read<Size>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x1f00'0000)) {
      beginBusTransaction(address, Size);
      address &= 0x1fff'ffff;
      auto data = bus.read<Size>(address);
      endBusTransaction();
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
      if(memory.cache.codeEnable == 1) {
        return icache.read(address);
      }
      if(memory.cache.scratchpadEnable == 1) {
        return scratchpad.read<Size>(address);
      }
      return 0;  //nop
    }
    if(likely(address <= 0x807f'ffff)) {
      beginBusTransaction(address, Size);
      auto data = ram.read<Size>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x9fc0'0000)) {
      beginBusTransaction(address, Size);
      auto data = bios.read<Size>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x9f00'0000)) {
      beginBusTransaction(address, Size);
      address &= 0x1fff'ffff;
      auto data = bus.read<Size>(address);
      endBusTransaction();
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
      beginBusTransaction(address, Size);
      auto data = ram.read<Size>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0xbfc0'0000)) {
      beginBusTransaction(address, Size);
      auto data = bios.read<Size>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0xbf00'0000)) {
      beginBusTransaction(address, Size);
      address &= 0x1fff'ffff;
      auto data = bus.read<Size>(address);
      endBusTransaction();
      return data;
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;
  }

  case 1:
  case 2:
  case 3:
  case 6:
  case 7: {
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;
  }

  }

  unreachable;
}

template<u32 Mode>
auto CPU::Breakpoint::testDataBus(u32 address) -> bool {
  return testData<Mode, Word>(address);
}

inline auto CPU::readWordPartial(u32 address, u32 lanes) -> u32 {
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(unlikely(breakpoint.active)) {
      u32 start = lanes & 1 ? address & ~3u : address;
      if(breakpoint.testData<Read, Byte>(start)) return 0;
    }
  }

  address &= ~3u;
  return readBusWordPartial(address, lanes);
}

inline auto CPU::writeWordPartial(u32 address, u32 data, u32 lanes) -> void {
  ioPoll = {};
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(unlikely(breakpoint.active)) {
      u32 start = lanes & 1 ? address & ~3u : address;
      if(breakpoint.testData<Write, Byte>(start)) return;
    }
  }

  address &= ~3u;
  return writeBus<Word>(address, data, lanes);
}

inline auto CPU::readBusWordPartial(u32 address, u32 lanes) -> u32 {
  const u32 virtualAddress = address;
  if(likely(!scc.status.cache.isolate && (address & 0x7fff'ffff) <= 0x007f'ffff)) {
    beginBusTransaction(virtualAddress, Word);
    auto data = ram.read<Word>(address);
    endBusTransaction();
    return data;
  }

  auto read16BitBus = [&]() -> u32 {
    if(lanes == 0b1111) return bus.read<Word>(address);
    bus.read<Half>(address + (lanes & 0b0011 ? 0 : 2));
    return bus.readLatch;
  };

  if(unlikely(address >= 0xfffe'0000)) {
    return memory.read<Word>(address);
  }

  switch(address >> 29) {
  case 0: {
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.codeEnable == 1) {
        return icache.read(address);
      }
      if(memory.cache.scratchpadEnable == 1) {
        return scratchpad.read<Word>(address);
      }
      return 0;
    }
    if(likely(address <= 0x007f'ffff)) {
      beginBusTransaction(virtualAddress, Word);
      auto data = ram.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x1fc0'0000)) {
      beginBusTransaction(virtualAddress, Word);
      auto data = bios.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x1f00'0000)) {
      address &= 0x1fff'ffff;
      if(address >= 0x1f80'1c00 && address <= 0x1f80'1fff && bus.busConfiguration(address).width == Half) {
        beginBusTransaction(virtualAddress, lanes == 0b1111 ? Word : Half);
        endBusTransaction();
        return read16BitBus();
      }
      beginBusTransaction(virtualAddress, Word);
      auto data = bus.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;
  }

  case 4: {
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.codeEnable == 1) {
        return icache.read(address);
      }
      if(memory.cache.scratchpadEnable == 1) {
        return scratchpad.read<Word>(address);
      }
      return 0;
    }
    if(likely(address <= 0x807f'ffff)) {
      beginBusTransaction(virtualAddress, Word);
      auto data = ram.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x9fc0'0000)) {
      beginBusTransaction(virtualAddress, Word);
      auto data = bios.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0x9f00'0000)) {
      address &= 0x1fff'ffff;
      if(address >= 0x1f80'1c00 && address <= 0x1f80'1fff && bus.busConfiguration(address).width == Half) {
        beginBusTransaction(virtualAddress, lanes == 0b1111 ? Word : Half);
        endBusTransaction();
        return read16BitBus();
      }
      beginBusTransaction(virtualAddress, Word);
      auto data = bus.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return 0;
  }

  case 5: {
    if(likely(address <= 0xa07f'ffff)) {
      beginBusTransaction(virtualAddress, Word);
      auto data = ram.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0xbfc0'0000)) {
      beginBusTransaction(virtualAddress, Word);
      auto data = bios.read<Word>(address);
      endBusTransaction();
      return data;
    }
    if(likely(address >= 0xbf00'0000)) {
      address &= 0x1fff'ffff;
      if(address >= 0x1f80'1c00 && address <= 0x1f80'1fff && bus.busConfiguration(address).width == Half) {
        beginBusTransaction(virtualAddress, lanes == 0b1111 ? Word : Half);
        endBusTransaction();
        return read16BitBus();
      }
      beginBusTransaction(virtualAddress, Word);
      auto data = bus.read<Word>(address);
      endBusTransaction();
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

template<u32 Size>
inline auto CPU::read(u32 address) -> u32 {
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(unlikely(breakpoint.active) && breakpoint.testData<Read, Size>(address)) return 0;
  }

  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) {
      if(unlikely(address & 1)) return exception.address<Read>(address), 0;
    }
    if constexpr(Size == Word) {
      if(unlikely(address & 3)) return exception.address<Read>(address), 0;
    }
  }

  return readBus<Size>(address);
}

template<u32 Size>
inline auto CPU::writeBus(u32 address, u32 data, u32 lanes) -> void {
  if(likely(!scc.status.cache.isolate && (address & 0x7fff'ffff) <= 0x007f'ffff)) {
    return queueWrite(address, data, Size, lanes);
  }

  auto writeScratchpad = [&]() -> void {
    if(likely(lanes == (1u << Size) - 1)) return scratchpad.write<Size>(address, data);
    for(u32 byte = 0; byte < Size; byte++) {
      if(lanes & 1u << byte) scratchpad.write<Byte>(address + byte, data >> (byte * 8));
    }
  };
  if(unlikely(address >= 0xfffe'0000)) {
    return memory.write<Size>(address, data);
  }

  switch(address >> 29) {
  //cached
  case 0: {//KUSEG
    if(unlikely(scc.status.cache.isolate)) {
      if(memory.cache.tagTest == 1 && memory.cache.codeEnable == 1) {
        return icache.writeTag(address, data);
      }
      if((memory.cache.invalidate || memory.cache.lock) && memory.cache.codeEnable == 1) {
        return icache.writeTag(address, 0);
      }
      if(memory.cache.codeEnable == 1) {
        return icache.writeData(address, data);
      }
      if(memory.cache.scratchpadEnable == 1) {
        return writeScratchpad();
      }
      return;
    }
    if(likely(address <= 0x007f'ffff)) {
      return queueWrite(address, data, Size, lanes);
    }
    if(likely(address >= 0x1fc0'0000)) {
      return queueWrite(address, data, Size, lanes);
    }
    if(likely(address >= 0x1f00'0000)) {
      return queueWrite(address, data, Size, lanes);
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
        return icache.writeTag(address, data);
      }
      if((memory.cache.invalidate || memory.cache.lock) && memory.cache.codeEnable == 1) {
        return icache.writeTag(address, 0);
      }
      if(memory.cache.codeEnable == 1) {
        return icache.writeData(address, data);
      }
      if(memory.cache.scratchpadEnable == 1) {
        return writeScratchpad();
      }
      return;
    }
    if(likely(address <= 0x807f'ffff)) {
      return queueWrite(address, data, Size, lanes);
    }
    if(likely(address >= 0x9fc0'0000)) {
      return queueWrite(address, data, Size, lanes);
    }
    if(likely(address >= 0x9f00'0000)) {
      return queueWrite(address, data, Size, lanes);
    }
    if constexpr(Accuracy::CPU::BusErrors) {
      exception.busData();
    }
    return;
  }

  //uncached
  case 5: {//KSEG1
    if(likely(address <= 0xa07f'ffff)) {
      return queueWrite(address, data, Size, lanes);
    }
    if(likely(address >= 0xbfc0'0000)) {
      return queueWrite(address, data, Size, lanes);
    }
    if(likely(address >= 0xbf00'0000)) {
      return queueWrite(address, data, Size, lanes);
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

template<u32 Size>
inline auto CPU::write(u32 address, u32 data, u32 lanes) -> void {
  ioPoll = {};
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(unlikely(breakpoint.active) && breakpoint.testData<Write, Size>(address)) return;
  }

  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) {
      if(unlikely(address & 1)) return exception.address<Write>(address);
    }
    if constexpr(Size == Word) {
      if(unlikely(address & 3)) return exception.address<Write>(address);
    }
  }

  return writeBus<Size>(address, data, lanes);
}
