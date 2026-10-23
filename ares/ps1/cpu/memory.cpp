//read code from the bus
inline auto CPU::fetch(u32 address) -> u32 {
  if(unlikely(statusUserMode() && (address & 0x8000'0000))) {
    return exception.address<Read>(address), 0;
  }

  switch(address >> 29) {
  case 0:  //KUSEG cached
  case 4:  //KSEG0 cached
    return icache.fetch(address);

  case 5: {  //KSEG1 uncached
    u32 physical = address & 0x1fff'ffff;
    auto access = memory.decodeRAM(physical);
    if(access.type != MemoryControl::RAMAccess::NotRAM) {
      if(access.type == MemoryControl::RAMAccess::Mapped) {
        waitWriteBuffer(access.offset, 15);
        stepBus(bus.calcAccessTime<false, false>(physical, Word));
        return ram.read<Word>(access.offset);
      }
      if(access.type == MemoryControl::RAMAccess::HighZ) return step(1), 0;
      if constexpr(Accuracy::CPU::BusErrors) exception.busInstruction();
      return 0;
    }
    if(physical >= 0x1fc0'0000) {
      if(&bus.mmio(physical) == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busInstruction();
        return 0;
      }
      stepBus(bus.calcAccessTime<false, false>(physical, Word));
      return bios.read<Word>(physical);
    }
    if(physical >= 0x1f00'0000) {
      debug(unhandled, "CPU::fetch");
      return 0;
    }
    if constexpr(Accuracy::CPU::BusErrors) exception.busInstruction();
    return 0;
  }

  case 1:
  case 2:
  case 3:
  case 6:
  case 7:
    if constexpr(Accuracy::CPU::BusErrors) exception.busInstruction();
    return 0;
  }
  unreachable;
}

//peek at the next instruction, does not consume cycles
inline auto CPU::peek(u32 address) -> u32 {
  u32 physical = address & 0x1fff'ffff;
  auto access = memory.decodeRAM(physical);
  if(access.type == MemoryControl::RAMAccess::Mapped) return ram.read<Word>(access.offset);
  if(access.type == MemoryControl::RAMAccess::HighZ) return 0;
  if(physical >= 0x1fc0'0000) return bios.read<Word>(physical);
  debug(unimplemented, "CPU::peek ", hex(address));
  return 0;
}

template<u32 Size>
inline auto CPU::readRAM(u32 address) -> u32 {
  u32 physical = address & 0x1fff'ffff;
  auto access = memory.decodeRAM(physical);
  if(access.type == MemoryControl::RAMAccess::Mapped) {
    u8 byteEnable = writeBufferByteEnable(access.offset, Size);
    waitWriteBuffer(access.offset, byteEnable);
    stepBus(bus.calcAccessTime<false, false>(physical, Size), Bus::CPUAccess, true);
    return ram.read<Size>(access.offset);
  }
  if(access.type == MemoryControl::RAMAccess::HighZ) return step(1), 0;
  if constexpr(Accuracy::CPU::BusErrors) exception.busData();
  return 0;
}

template<u32 Size>
inline auto CPU::writeRAM(u32 address, u32 data) -> void {
  u32 physical = address & 0x1fff'ffff;
  auto access = memory.decodeRAM(physical);
  if(access.type == MemoryControl::RAMAccess::Mapped) {
    return enqueueWriteBuffer(physical, access.offset, data, writeBufferByteEnable(access.offset, Size));
  }
  if(access.type == MemoryControl::RAMAccess::HighZ) return step(1);
  if constexpr(Accuracy::CPU::BusErrors) exception.busData();
}

template<u32 Size>
inline auto CPU::read(u32 address) -> u32 {
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(testDataBreakpoint<Read, Size>(address)) return 0;
  }
  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) if(unlikely(address & 1)) return exception.address<Read>(address), 0;
    if constexpr(Size == Word) if(unlikely(address & 3)) return exception.address<Read>(address), 0;
  }
  if(unlikely(statusUserMode() && (address & 0x8000'0000))) {
    return exception.address<Read>(address), 0;
  }
  if(unlikely(address >= 0xfffe'0000)) return memory.read<Size>(address);

  switch(address >> 29) {
  case 0:
  case 4: {  //KUSEG/KSEG0 cached
    u32 physical = address & 0x1fff'ffff;
    if(unlikely(statusCacheIsolated())) {
      u32 word = 0;
      if(memory.cache.codeEnable) {
        word = memory.cache.tagTest ? icache.readTag(physical) : icache.read(physical);
      } else if(memory.cache.scratchpadEnable) {
        return scratchpad.read<Size>(physical);
      } else {
        return 0;
      }
      if constexpr(Size == Byte) return word >> (physical & 3) * 8 & 0xff;
      if constexpr(Size == Half) return word >> (physical & 2) * 8 & 0xffff;
      return word;
    }

    auto ramAccess = memory.decodeRAM(physical);
    if(ramAccess.type != MemoryControl::RAMAccess::NotRAM) {
      if(!memory.cache.scratchpadEnable && memory.cache.dataEnable
      && ramAccess.type == MemoryControl::RAMAccess::Mapped) {
        waitWriteBuffer(ramAccess.offset, 15);
        stepBus(bus.calcAccessTime<false, false>(physical, Word), Bus::CPUAccess, true);
        u32 word = ram.read<Word>(ramAccess.offset & ~3);
        scratchpad.write<Word>(physical & 0x3fc, word);
        if constexpr(Size == Byte) return word >> (physical & 3) * 8 & 0xff;
        if constexpr(Size == Half) return word >> (physical & 2) * 8 & 0xffff;
        return word;
      }
      return readRAM<Size>(physical);
    }

    if(physical >= 0x1fc0'0000) {
      if(&bus.mmio(physical) == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return 0;
      }
      stepBus(bus.calcAccessTime<false, false>(physical, Size));
      return bios.read<Size>(physical);
    }
    if(physical >= 0x1f00'0000) {
      if((physical & 0xffff'fc00) == 0x1f80'0000
      && !memory.cache.scratchpadEnable && memory.cache.dataEnable) {
        enterBusWait(MemoryFrontend::BusWaitDCacheScratchpad);
        return 0;
      }
      if((physical & 0xffff'fc00) == 0x1f80'0000) {
        step(1);
        return scratchpad.read<Size>(physical);
      }
      auto& target = bus.mmio(physical);
      if(&target == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return 0;
      }
      stepBus(bus.calcAccessTime<false, false>(physical, Size));
      return target.read<Size>(physical);
    }
    if constexpr(Accuracy::CPU::BusErrors) exception.busData();
    return 0;
  }

  case 5: {  //KSEG1 uncached
    u32 physical = address & 0x1fff'ffff;
    if(memory.decodeRAM(physical).type != MemoryControl::RAMAccess::NotRAM) return readRAM<Size>(physical);
    if(physical >= 0x1fc0'0000) {
      if(&bus.mmio(physical) == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return 0;
      }
      stepBus(bus.calcAccessTime<false, false>(physical, Size));
      return bios.read<Size>(physical);
    }
    if(physical >= 0x1f00'0000) {
      if((physical & 0xffff'fc00) == 0x1f80'0000) {
        step(1);
        return scratchpad.read<Size>(physical);
      }
      auto& target = bus.mmio(physical);
      if(&target == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return 0;
      }
      stepBus(bus.calcAccessTime<false, false>(physical, Size));
      return target.read<Size>(physical);
    }
    if constexpr(Accuracy::CPU::BusErrors) exception.busData();
    return 0;
  }

  case 1:
  case 2:
  case 3:
  case 6:
  case 7:
    if constexpr(Accuracy::CPU::BusErrors) exception.busData();
    return 0;
  }
  unreachable;
}

template<u32 Size>
inline auto CPU::write(u32 address, u32 data) -> void {
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(testDataBreakpoint<Write, Size>(address)) return;
  }
  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) if(unlikely(address & 1)) return exception.address<Write>(address);
    if constexpr(Size == Word) if(unlikely(address & 3)) return exception.address<Write>(address);
  }
  if(unlikely(statusUserMode() && (address & 0x8000'0000))) {
    return exception.address<Write>(address);
  }
  if(unlikely(address >= 0xfffe'0000)) return memory.write<Size>(address, data);

  switch(address >> 29) {
  case 0:
  case 4: {  //KUSEG/KSEG0 cached
    u32 physical = address & 0x1fff'ffff;
    if(unlikely(statusCacheIsolated())) {
      if(memory.cache.codeEnable) {
        if(memory.cache.tagTest) return icache.writeTag(physical, data);
        u32 shifted = data << (physical & 3) * 8;
        return icache.write(physical, shifted, writeBufferByteEnable(physical, Size));
      }
      if(memory.cache.scratchpadEnable) return scratchpad.write<Size>(physical, data);
      return;
    }

    if(memory.decodeRAM(physical).type != MemoryControl::RAMAccess::NotRAM) return writeRAM<Size>(physical, data);
    if(physical >= 0x1fc0'0000) {
      if(&bus.mmio(physical) == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return;
      }
      stepBus(bus.calcAccessTime<true, false>(physical, Size));
      return bios.write<Size>(physical, data);
    }
    if(physical >= 0x1f00'0000) {
      if((physical & 0xffff'fc00) == 0x1f80'0000
      && !memory.cache.scratchpadEnable && memory.cache.dataEnable) {
        return enterBusWait(MemoryFrontend::BusWaitDCacheScratchpad);
      }
      if((physical & 0xffff'fc00) == 0x1f80'0000) {
        step(1);
        return scratchpad.write<Size>(physical, data);
      }
      auto& target = bus.mmio(physical);
      if(&target == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return;
      }
      stepBus(bus.calcAccessTime<true, false>(physical, Size));
      return target.write<Size>(physical, data);
    }
    if constexpr(Accuracy::CPU::BusErrors) exception.busData();
    return;
  }

  case 5: {  //KSEG1 uncached
    u32 physical = address & 0x1fff'ffff;
    if(memory.decodeRAM(physical).type != MemoryControl::RAMAccess::NotRAM) return writeRAM<Size>(physical, data);
    if(physical >= 0x1fc0'0000) {
      if(&bus.mmio(physical) == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return;
      }
      stepBus(bus.calcAccessTime<true, false>(physical, Size));
      return bios.write<Size>(physical, data);
    }
    if(physical >= 0x1f00'0000) {
      if((physical & 0xffff'fc00) == 0x1f80'0000) {
        step(1);
        return scratchpad.write<Size>(physical, data);
      }
      auto& target = bus.mmio(physical);
      if(&target == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) exception.busData();
        return;
      }
      stepBus(bus.calcAccessTime<true, false>(physical, Size));
      return target.write<Size>(physical, data);
    }
    if constexpr(Accuracy::CPU::BusErrors) exception.busData();
    return;
  }

  case 1:
  case 2:
  case 3:
  case 6:
  case 7:
    if constexpr(Accuracy::CPU::BusErrors) exception.busData();
    return;
  }
}
