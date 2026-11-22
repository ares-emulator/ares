
auto CPU::writeQueueWindow(u32 clocks) -> u32 {
  writeQueue.stalled = false;

  if(busAccess) return clocks;

  if(dma.active()) {
    writeQueue.page = ~0u;
    writeQueue.stalled = true;
    return min<u32>(clocks, min<u32>(dmaStallGranularity, dma.cpuReleaseWindow()));
  }

  if(!writeQueue.remaining) {
    if(!writeQueue.count) return clocks;

    auto entry = writeQueue.entries[writeQueue.head];
    if(entry.address <= 0x007f'ffff) {
      if(entry.size == Word && entry.lanes == 15) {
        u32 page = (entry.address & ram.maskByte) & ~0x3ffu;
        writeQueue.remaining = writeQueue.page == page ? 2 : 4;
        writeQueue.page = page;
      } else {
        writeQueue.remaining = 4;
        writeQueue.page = ~0u;
      }
    } else {
      writeQueue.remaining = bus.calcAccessTime<true, false>(entry.address, entry.size);
      writeQueue.page = ~0u;
    }

    writeQueue.head = (writeQueue.head + 1) & 3;
    writeQueue.count--;
    commitWrite(entry.address, entry.data, entry.size, entry.lanes);
  }
  return min<u32>(clocks, writeQueue.remaining);
}

auto CPU::writeQueueAdvance(u32 clocks) -> void {
  if(writeQueue.stalled || busAccess || !writeQueue.remaining) return;
  if(writeQueue.remaining -= clocks) return;
  if(!writeQueue.count) writeQueue.page = ~0u;
}

auto CPU::flushWriteQueue() -> void {
  while(writeQueue.count || writeQueue.remaining) {
    step(writeQueue.remaining ? writeQueue.remaining : 4);
  }
}

alwaysinline auto CPU::beginBusTransaction(u32 address, u32 size, bool instruction) -> void {
  if(unlikely(readRemaining)) step(readRemaining);
  u32 physical = address & 0x1fff'ffff;
  if(likely(physical <= 0x007f'ffff && !(writeQueue.count | writeQueue.remaining))) {
    if(likely(!dma.active() && (!dma.enabledMask || clock() + (u64)accruedCycles * scalar() < dma.clock()))) {
      busAccess = true;
      writeQueue.page = ~0u;
      busTransactionClocks = instruction ? 3 + ((size + 3) / 4) : Bus::ramReadAccessTime;
      return;
    }
  }

  if(unlikely(address >> 29 == 5)) {
    if(writeQueue.count || writeQueue.remaining) flushWriteQueue();
  }
  if(unlikely((physical & 0xffff'fc00) == 0x1f80'0000)) return;

  if(unlikely(writeQueue.count)) {
    auto hazardAddress = [&](u32 value) -> u32 {
      if(value <= 0x007f'ffff) value &= ram.maskByte;
      return value & ~63u;
    };
    u32 first = hazardAddress(physical);
    u32 last  = hazardAddress(physical + size - 1);
    for(u32 index = 0; index < writeQueue.count; index++) {
      auto& entry = writeQueue.entries[(writeQueue.head + index) & 3];
      u32 target = hazardAddress(entry.address);
      if(target >= first && target <= last) { flushWriteQueue(); break; }
    }
  }
  if(unlikely(writeQueue.remaining)) step(writeQueue.remaining);

  if(unlikely(dma.enabledMask && !dma.active())) {
    if(clock() + (u64)accruedCycles * scalar() >= dma.clock()) {
      for(u32 id : dma.channelsByPriority) {
        if(!(dma.enabledMask & (1 << id))) continue;
        if(dma.channels[id].kick()) break;
      }
    }
  }
  busAccess = true;
  if(unlikely(dma.active())) waitDMA();
  writeQueue.page = ~0u;
  if(likely(physical <= 0x007f'ffff)) {
    busTransactionClocks = instruction ? 3 + ((size + 3) / 4) : Bus::ramReadAccessTime;
  } else {
    busTransactionClocks = bus.calcAccessTime<false, false>(physical, size);
  }
}

alwaysinline auto CPU::endBusTransaction(bool overlap) -> void {
  if(!busAccess) return;
  //The last two read-response clocks can overlap independent instructions.
  //Keep the bus reserved until they expire, including across a cache miss.
  u32 tail = overlap && memory.cache.loadScheduling && busTransactionClocks > 2 ? min(2u, busTransactionClocks - 2) : 0;
  step(busTransactionClocks - tail);
  readRemaining = tail;
  if(tail || (overlap && memory.cache.loadScheduling && busTransactionClocks == 2)) {
    readLatency = tail ? tail + 1 : 3;
    readWritebackRemaining = readLatency + 1;
    u32 opcode = pipeline.instruction >> 26;
    readTarget = opcode >= 0x20 && opcode <= 0x26 ? pipeline.instruction >> 16 & 31 : 0;
  }
  if(!tail) busAccess = false;
}

auto CPU::queueWrite(u32 address, u32 data, u32 size, u32 lanes) -> void {
  u32 physical = address & 0x1fff'ffff;
  bool buffered = address >> 29 != 5;
  if(!buffered && (writeQueue.count || writeQueue.remaining)) flushWriteQueue();

  if((physical & 0xffff'fc00) == 0x1f80'0000) {
    step(1);
    commitWrite(physical, data, size, lanes);
    return;
  }
  if(!buffered) {
    if(unlikely(readRemaining)) step(readRemaining);
    busAccess = true;
    if(dma.active()) waitDMA();
    writeQueue.page = ~0u;

    commitWrite(physical, data, size, lanes);
    step(bus.calcAccessTime<true, false>(physical, size));
    busAccess = false;
    return;
  }
  while(writeQueue.count == 4) step(1);
  auto& entry = writeQueue.entries[(writeQueue.head + writeQueue.count) & 3];
  entry = {physical, data, size, lanes};
  writeQueue.count++;
}

auto CPU::commitWrite(u32 address, u32 data, u32 size, u32 lanes) -> void {
  auto writeMemory = [&](auto& memory) -> void {
    if(size == Byte) return memory.template write<Byte>(address, data);
    if(size == Half) return memory.template write<Half>(address, data);
    if(likely(lanes == 15)) return memory.template write<Word>(address, data);
    for(u32 byte : range(4)) {
      if(lanes & 1u << byte) memory.template write<Byte>(address + byte, data >> byte * 8);
    }
  };
  if(likely(address <= memory.ramLimit)) {
    return writeMemory(ram);
  }
  if(address <= 0x007f'ffff) return;

  if(address >= 0x1fc0'0000) return writeMemory(bios);

  if(size == Byte) return bus.write<Byte>(address, data);
  if(size == Half) return bus.write<Half>(address, data);
  if(lanes == 15) return bus.write<Word>(address, data);
  switch(lanes) {
  case 1: return bus.write<Byte>(address, data);
  case 3: return bus.write<Half>(address, data);
  case 7:
    bus.write<Half>(address, data);
    return bus.write<Byte>(address + 2, data >> 16);
  case 8: return bus.write<Byte>(address + 3, data >> 24);
  case 12: return bus.write<Half>(address + 2, data >> 16);
  case 14:
    bus.write<Byte>(address + 1, data >> 8);
    return bus.write<Half>(address + 2, data >> 16);
  }
}
