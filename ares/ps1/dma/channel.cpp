auto DMA::sortChannelsByPriority() -> void {
  n1 selected[7] = {};
  for(u32 index : range(7)) {
    u32 id = 8, lowest = 8;
    for(u32 search : range(7)) {
      if(selected[search]) continue;
      if(channels[search].priority <= lowest) {
        lowest = channels[search].priority;
        id = search;
      }
    }
    selected[id] = 1;
    channelsByPriority[index] = id;
  }
}

auto DMA::Channel::ready() -> bool {
  static bool (* const canRead[7])() = {
    []() { return false; },
    []() { return mdec.canReadDMA(); },
    []() { return gpu.canReadDMA(); },
    []() { return disc.canReadDMA(); },
    []() { return spu.canReadDMA(); },
    []() { return false; },
    []() { return true; },
  };

  static bool (* const canWrite[7])() = {
    []() { return mdec.canWriteDMA(); },
    []() { return false; },
    []() { return gpu.canWriteDMA(); },
    []() { return false; },
    []() { return spu.canWriteDMA(); },
    []() { return false; },
    []() { return false; },
  };

  return direction == 0 ? canRead[id]() : canWrite[id]();
}

auto DMA::Channel::acceptRequest() -> bool {
  bool request = ready();
  if(id == OTC && synchronization == 0) request = false;  //OTC has no external DREQ.
  if(!request && !trigger) return false;
  if(!request && synchronization == 0 && unknown.bit(0)) return false;
  forced = !request;
  if(request || synchronization == 0 || !unknown.bit(0)) trigger = 0;
  return true;
}

auto DMA::Channel::kick() -> bool {
  if(!masterEnable || !enable || state != Idle) return false;
  //Undefined protocols remain guest waits; never reinterpret them or abort the host.
  if(synchronization == 3 || (synchronization == 2 && !direction)) return false;
  if(synchronization == 1 && chopping.enable) return false;
  if(!acceptRequest()) return false;

  baseAddress = address;
  baseLength = length;
  state = Running;
  blockOffset = 0;
  chopping.remaining = 1 << chopping.dmaWindow;
  if(synchronization != 2) chain = {};
  return true;
}

auto DMA::Channel::step() -> bool {
  u8 owner = Bus::dmaOwner(id);
  if(!masterEnable || !enable) {
    bus.release(owner);
    return false;
  }
  if(state == Idle && !kick()) return false;

  bool resumedTransaction = bus.arbiter.owner == owner;
  if(!resumedTransaction) {
    if(synchronization == 1 && chopping.enable) return false;
    if(synchronization == 0 && forced && unknown.bit(0)) return false;
    //Non-GPU linked endpoints accept the whole node, as in the reference DMA path.
    //GPU payloads retain per-word FIFO readiness rather than adopting host-side batching.
    if(synchronization == 2 && id == GPU) {
      if(!forced && !ready()) return false;
    }
    if(!bus.acquire(owner)) return false;
    dma.step(transferClocks());
  }
  //Check the effective RAM address before consuming device data or a list word.
  u32 target = address & 0xfffffc;
  if(id == MDECout && direction == 0) target = (target + mdec.dmaOffset() * 4) & 0xfffffc;
  auto access = memory.decodeRAM(target);
  if(access.type == MemoryControl::RAMAccess::Unmapped || access.type == MemoryControl::RAMAccess::NotRAM) {
    enable = 0;
    state = Idle;
    dma.irq.force = 1;
    irq.flag = 1;
    bus.release(owner);
    dma.irq.poll();
    return true;
  }
  if(synchronization == 2) transferChain();
  else transferBlock();
  bus.release(owner, !enable || state == Idle || dma.counter > 0);

  if(!enable) {
    state = Idle;
    signalIRQ();
  } else if((synchronization == 1 && state == Idle) || (synchronization == 2 && !chain.length)) {
    signalIRQ(true);
  }
  return true;
}

auto DMA::Channel::signalIRQ(bool segment) -> void {
  if(!dma.irq.enable || !irq.enable) return;
  if(segment && !(dma.irq.unknown >> id & 1)) return;
  irq.flag = 1;
  dma.irq.poll();
}

auto DMA::Channel::transferClocks() -> u32 {
  //Reference timing: a list header is its own phase, not an ordinary payload word.
  if(synchronization == 2 && !chain.length) return 8;
  u32 address = this->address & 0xfffffc;
  if(id == MDECout && direction == 0) address = (address + mdec.dmaOffset() * 4) & 0xfffffc;
  u32 clocks = 0;
  if(memory.decodeRAM(address).type == MemoryControl::RAMAccess::Mapped) {
    u32 sequence = blockOffset;
    clocks = 1 + ((sequence & 15) == 0);  //one row-open cycle per 16-word DMA burst
  } else {
    clocks = direction == 0
      ? bus.calcAccessTime<true, true>(address, Word)
      : bus.calcAccessTime<false, true>(address, Word);
  }
  if(synchronization == 2) return clocks + (blockOffset == 0 ? 5 : 0);

  if(direction == 0) {
    switch(id) {
    case MDECout: clocks += bus.calcAccessTime<false, true>(0x1f80'1820, Word); break;
    case GPU:     clocks += bus.calcAccessTime<false, true>(0x1f80'1810, Word); break;
    case CDROM:   clocks += bus.calcAccessTime<false, true>(0x1f80'1802, Word); break;
    case SPU:     clocks += bus.calcAccessTime<false, true>(0x1f80'1c00, Word); break;
    }
  } else {
    switch(id) {
    case MDECin: clocks += bus.calcAccessTime<true, true>(0x1f80'1820, Word); break;
    case GPU:    clocks += bus.calcAccessTime<true, true>(0x1f80'1810, Word); break;
    case SPU:    clocks += bus.calcAccessTime<true, true>(0x1f80'1c00, Word); break;
    }
  }
  return clocks;
}

auto DMA::Channel::transferBlock() -> void {
  if(blockOffset == 0) dma.debugger.transfer(id);

  u32 address = this->address & 0xfffffc;
  u32 targetAddress = id == MDECout && direction == 0
    ? (address + mdec.dmaOffset() * 4) & 0xfffffc : address;
  s32 stride = decrement ? -4 : +4;

  if(direction == 0) {
    u32 data = 0xffff'ffff;
    switch(id) {
    case MDECin:  debug(unimplemented, "DMA MDECin read"); break;
    case MDECout: data = mdec.readDMA(); break;
    case GPU:     data = gpu.readDMA(); break;
    case CDROM:   data = disc.readDMA(); break;
    case SPU:     data = spu.readDMA(); break;
    case PIO:     debug(unimplemented, "DMA PIO read"); break;
    case OTC:     data = (length == 1) ? 0xff'ffff : (address - 4 & 0xff'ffff); break;
    }
    bus.write<Word>(targetAddress, data);
  } else {
    u32 data = bus.read<Word>(address);
    switch(id) {
    case MDECin:  mdec.writeDMA(data); break;
    case MDECout: debug(unimplemented, "DMA MDECout write"); break;
    case GPU:     gpu.writeDMA(data); break;
    case CDROM:   debug(unimplemented, "DMA CDROM write"); break;
    case SPU:     spu.writeDMA(data); break;
    case PIO:     debug(unimplemented, "DMA PIO write"); break;
    case OTC:     debug(unimplemented, "DMA OTC write"); break;
    }
  }

  this->address = address + stride;

  if(synchronization == 0) {
    length--;
    blockOffset++;
    if(chopping.enable) {
      baseAddress = this->address;
      baseLength = length;
    }
    if(length == 0) {
      enable = 0;
      return;
    }
    if(chopping.enable && --chopping.remaining == 0) {
      chopping.remaining = 1 << chopping.dmaWindow;
      dma.counter = 1 << chopping.cpuWindow;
    }
    return;
  }

  u32 blockWords = length ? (u32)length : 0x1'0000;
  if(++blockOffset < blockWords) return;
  blockOffset = 0;
  baseAddress = this->address;
  blocks--;
  if(blocks == 0) enable = 0;
  else if(synchronization == 1) state = Idle;
}

auto DMA::Channel::transferChain() -> void {
  if(chain.transferred == 0) dma.debugger.transfer(id);

  u32 address = this->address & 0xfffffc;
  if(!chain.length) {
    n32 header = bus.read<Word>(address);
    chain.address = header >> 0;
    chain.length = header >> 24;
    chain.transferred++;
    blockOffset = 0;
    this->address = address + 4;

    if(!chain.length) {
      this->address = baseAddress = chain.address == 0xffffff ? 0xffffff : chain.address & 0xfffffc;
      state = Idle;
      if(chain.address == 0xffffff) enable = 0;
    }
    return;
  }

  u32 data = bus.read<Word>(address);
  switch(id) {
  case MDECin:  mdec.writeDMA(data); break;
  case MDECout: debug(unimplemented, "DMA MDECout chain"); break;
  case GPU:     gpu.writeDMA(data); break;
  case CDROM:   debug(unimplemented, "DMA CDROM chain"); break;
  case SPU:     spu.writeDMA(data); break;
  case PIO:     debug(unimplemented, "DMA PIO chain"); break;
  case OTC:     debug(unimplemented, "DMA OTC chain"); break;
  }

  this->address = address + 4;
  chain.length--;
  chain.transferred++;
  blockOffset++;
  if(!chain.length) {
    this->address = baseAddress = chain.address == 0xffffff ? 0xffffff : chain.address & 0xfffffc;
    state = Idle;
    if(chain.address == 0xffffff) enable = 0;
  }
}
