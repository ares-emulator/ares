//used by the interpreter to fully emulate the instruction cache
inline auto CPU::InstructionCache::fetch(u32 address) -> u32 {
  u32 physical = address & 0x1fff'ffff;
  u32 word = address >> 2 & 3;
  auto& line = lines[address >> 4 & 0xff];

  if(!memory.cache.codeEnable) {
    if(auto access = memory.decodeRAM(physical); access.type != MemoryControl::RAMAccess::NotRAM) {
      cpu.stepBus(bus.calcAccessTime<false, false>(physical, Word));
      if(access.type == MemoryControl::RAMAccess::Mapped) return cpu.ram.read<Word>(access.offset);
      if(access.type == MemoryControl::RAMAccess::HighZ) return 0;
      if constexpr(Accuracy::CPU::BusErrors) cpu.exception.busInstruction();
      return 0;
    }
    if(physical >= 0x1fc0'0000) {
      if(&bus.mmio(physical) == &unmapped) {
        if constexpr(Accuracy::CPU::BusErrors) cpu.exception.busInstruction();
        return 0;
      }
      cpu.stepBus(bus.calcAccessTime<false, false>(physical, Word));
      return bios.read<Word>(physical);
    }
    if constexpr(Accuracy::CPU::BusErrors) cpu.exception.busInstruction();
    return 0;
  }

  if(line.tag != (physical & 0x1fff'f000) || !(line.valid & 1 << word)) {
    if(!startRefill(address)) return 0;
    while(cpu.frontend.refill.active) {
      cpu.step(cpu.frontend.refill.completion - cpu.execution.clock);
    }
  } else {
    cpu.step(1);
  }
  return line.words[word];
}

inline auto CPU::InstructionCache::read(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 0xff];
  return line.words[address >> 2 & 3];
}

inline auto CPU::InstructionCache::write(u32 address, u32 data, u8 byteEnable) -> void {
  auto& word = lines[address >> 4 & 0xff].words[address >> 2 & 3];
  for(u32 byte : range(4)) {
    if(byteEnable & 1 << byte) {
      u32 mask = 0xff << byte * 8;
      word = word & ~mask | data & mask;
    }
  }
}

inline auto CPU::InstructionCache::readTag(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 0xff];
  u32 match = line.tag == (address & 0x1fff'f000);
  return line.words[address >> 2 & 3] & ~0x1f | match << 4 | line.valid;
}

inline auto CPU::InstructionCache::writeTag(u32 address, u32 data) -> void {
  auto& line = lines[address >> 4 & 0xff];
  line.tag = address & 0x1fff'f000;
  line.valid = data & 15;
}

auto CPU::InstructionCache::refillLatency(u32 words) const -> u32 {
  u32 address = cpu.frontend.refill.address;
  if(memory.decodeRAM(address).type != MemoryControl::RAMAccess::NotRAM) return 4 + words;
  if((address & 0x1fff'ffff) >= 0x1fc0'0000) {
    return memory.bios.calcAccessTime<false, false>(words * Word);
  }
  return words;
}

auto CPU::InstructionCache::startRefill(u32 address) -> bool {
  if(memory.cache.codeSize > 1) {
    cpu.enterBusWait(MemoryFrontend::BusWaitInvalidInstructionBlockSize);
    return false;
  }

  auto& refill = cpu.frontend.refill;
  u32 physical = address & 0x1fff'ffff;
  u8 word = address >> 2 & 3;
  auto& line = lines[address >> 4 & 0xff];
  u32 tag = physical & 0x1fff'f000;
  u8 finalWord = word == 0 && memory.cache.codeSize == 0 ? 1 : 3;

  for(u32 refillWord = word; refillWord <= finalWord; refillWord++) {
    auto access = memory.decodeRAM((physical & ~0xf) | refillWord * Word);
    if(access.type == MemoryControl::RAMAccess::Mapped) cpu.waitWriteBuffer(access.offset, 15);
  }

  if(line.tag != tag) {
    line.tag = tag;
    line.valid = 0;
  }

  refill.active = true;
  refill.address = physical & ~0xf;
  refill.tag = tag;
  refill.line = address >> 4 & 0xff;
  refill.requestedWord = word;
  refill.nextWord = word;
  refill.finalWord = finalWord;
  refill.completedWords = 0;
  cpu.waitBus(Bus::InstructionRefill);
  refill.started = cpu.execution.clock;
  refill.granted = true;
  refill.completion = refill.started + refillLatency(1);
  return true;
}

auto CPU::InstructionCache::retireRefill() -> void {
  auto& refill = cpu.frontend.refill;
  if(!refill.active) return;

  if(!refill.granted) {
    if(!bus.acquire(Bus::InstructionRefill)) {
      refill.completion = cpu.execution.clock + 1;
      return;
    }
    refill.granted = true;
    u32 previous = refill.completedWords ? refillLatency(refill.completedWords) : 0;
    refill.started = cpu.execution.clock - previous;
    refill.completion = refill.started + refillLatency(refill.completedWords + 1);
    return;
  }
  if(refill.completion > cpu.execution.clock) return;

  u32 address = refill.address | refill.nextWord * Word;
  u32 data = 0;
  auto access = memory.decodeRAM(address);
  if(access.type == MemoryControl::RAMAccess::Mapped) {
    data = cpu.ram.read<Word>(access.offset);
  } else if(access.type == MemoryControl::RAMAccess::HighZ) {
    data = 0;
  } else if(access.type == MemoryControl::RAMAccess::Unmapped) {
    if constexpr(Accuracy::CPU::BusErrors) cpu.exception.busInstruction();
    bus.release(Bus::InstructionRefill);
    refill.active = false;
    refill.completion = 0;
    refill.granted = false;
    return;
  } else if(address >= 0x1fc0'0000) {
    if(&bus.mmio(address) != &unmapped) {
      data = bios.read<Word>(address);
    } else {
      if constexpr(Accuracy::CPU::BusErrors) cpu.exception.busInstruction();
      bus.release(Bus::InstructionRefill);
      refill.active = false;
      refill.completion = 0;
      refill.granted = false;
      return;
    }
  } else {
    if constexpr(Accuracy::CPU::BusErrors) cpu.exception.busInstruction();
    bus.release(Bus::InstructionRefill);
    refill.active = false;
    refill.completion = 0;
    refill.granted = false;
    return;
  }

  auto& line = lines[refill.line];
  line.words[refill.nextWord] = data;
  line.valid |= 1 << refill.nextWord;
  refill.completedWords++;
  bus.release(Bus::InstructionRefill);
  refill.granted = false;
  if(refill.nextWord == refill.finalWord) {
    refill.active = false;
    refill.completion = 0;
    return;
  }
  refill.nextWord++;
  if(bus.acquire(Bus::InstructionRefill)) {
    refill.granted = true;
    refill.completion = refill.started + refillLatency(refill.completedWords + 1);
  } else {
    refill.completion = cpu.execution.clock + 1;
  }
}

auto CPU::InstructionCache::power(bool reset) -> void {
  for(auto& line : lines) {
    line.tag = random();
    line.valid = 0;
    for(auto& word : line.words) word = random();
  }
}
