auto DMA::sortChannelsByPriority() -> void {
  n1 selected[7];
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

auto DMA::Channel::dmaRequest() const -> bool {
  if(trigger) return true;
  if(direction == 0) {
    switch(id) {
    case 1: return mdec.canReadDMA();
    case 2: return gpu.canReadDMA();
    case 3: return disc.canReadDMA();
    case 4: return spu.canReadDMA();
    }
  } else {
    switch(id) {
    case 0: return mdec.canWriteDMA();
    case 2: return gpu.canWriteDMA();
    case 4: return spu.canWriteDMA();
    }
  }
  return false;
}

auto DMA::Channel::kick() -> bool {
  if(cpu.busAccess) return false;
  if(!masterEnable) return false;
  if(!enable) return false;

  if(state == Idle) {
    if(!dmaRequest()) return false;

    if(synchronization != 0) trigger = 0;
    state = Running;
    dma.runningMask |= 1 << id;
    return true;
  }

  return false;
}

auto DMA::Channel::step() -> bool {
  if(!masterEnable || !enable) {
    if(state == Running) {
      state = Idle;
      dma.runningMask &= ~(1 << id);
    }
    return false;
  }
  if(state == Idle && cpu.busAccess) {
    dma.blocked = true;
    return false;
  }

  if(state == Idle) {
    if(!dmaRequest()) {
      dma.blocked = true;
      return false;
    }

    if(synchronization != 0) trigger = 0;
    state = Running;
    dma.runningMask |= 1 << id;
  }

  switch(synchronization) {
    case 0: transferBlock(); break;
    case 1: transferBlock(); break;
    case 2: transferChain(); break;
  }

  if(!enable) {
    if(irq.enable) {
      irq.flag = 1;
      dma.irq.poll();
    }
  }

  state = Idle;
  dma.runningMask &= ~(1 << id);
  if(synchronization == 0 && chopping.enable == 1) {
    dma.counter = 1 << chopping.cpuWindow;
    dma.step(dma.counter);
  }

  return true;
}

auto DMA::Channel::transferBlock() -> void {
  dma.debugger.transfer(id);

  u32 address = this->address & ~3;
  u32 startingAddress = address;
  u16 length  = this->length;
  u32 timeout = synchronization == 0 && chopping.enable ? 1 << chopping.dmaWindow : ~0;
  s16 step    = this->decrement ? -4 : +4;
  u32 clocks  = 0;
  u32 wordsTransferred = 0;

  auto readWord = [&](u32 address) -> u32 {
    if((address & 0x1fff'ffff) <= 0x007f'ffff) return cpu.ram.readWord(address);
    return bus.read<Word>(address);
  };

  auto writeWord = [&](u32 address, u32 data) -> void {
    if((address & 0x1fff'ffff) <= 0x007f'ffff) return cpu.ram.writeWord(address, data);
    bus.write<Word>(address, data);
  };

  if(direction == 0) {
    switch(id) {
    case 1:
      do {
        u32 destination = mdec.dmaAddress(address);
        writeWord(destination, mdec.readDMA());
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    case 2:
      do {
        writeWord(address, gpu.readDMA());
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    case 3:
      do {
        writeWord(address, disc.readDMA());
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    case 4:
      do {
        writeWord(address, spu.readDMA());
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    case 6:
      do {
        u32 data = (address - 4) & 0xfffffc;
        if(length == 1) data = 0xffffff;
        writeWord(address, data);
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    default:
      break;
    }
  } else {
    switch(id) {
    case 0:
      do {
        mdec.writeDMA(readWord(address));
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    case 2:
      do {
        gpu.writeDMA(readWord(address));
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    case 4:
      do {
        spu.writeDMA(readWord(address));
        address += step;
        wordsTransferred++;
      } while(--length && --timeout);
      break;
    default:
      break;
    }
  }

  u32 bytes = wordsTransferred * 4;

  if(direction == 0) { // DMA -> CPU
    switch(id) {
    case 1: // MDECout
      clocks += bus.calcAccessTime<true,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<false,true>(0x1f801820, bytes);
      break;
    case 2: // GPU
      clocks += bus.calcAccessTime<true,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<false,true>(0x1f801810, bytes);
      break;
    case 3: // CDROM
      clocks += bus.calcAccessTime<true,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<false,true>(0x1f801802, bytes);
      break;
    case 4: // SPU
      clocks += bus.calcAccessTime<true,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<false,true>(0x1f801c00, bytes);
      break;
    case 6: // OTC
      clocks += bus.calcAccessTime<true,true>(startingAddress, bytes);
      break;
    }
  } else { // CPU -> DMA
    switch(id) {
    case 0: // MDECin
      clocks += bus.calcAccessTime<false,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<true,true>(0x1f801820, bytes);
      break;
    case 2: // GPU
      clocks += max(bus.calcAccessTime<false,true>(startingAddress, bytes),
                    bus.calcAccessTime<true,true>(0x1f801810, bytes));
      clocks += bus.calcAccessTime<true,true>(0x1f801810, 4);
      break;
    case 4: // SPU
      clocks += bus.calcAccessTime<false,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<true,true>(0x1f801c00, bytes);
      break;
    }
  }

  dma.counter = clocks;
  dma.step(clocks);

  if(synchronization == 0) {
    if(chopping.enable == 1) {
      this->length = length;
      this->address = address;
    }

    if(length == 0) {
      enable = 0;
      trigger = 0;
      dma.enabledMask &= ~(1 << id);
    }
    return;
  }

  this->address = address;
  if(length == 0) {
    this->blocks--;
    if(blocks == 0) {
      enable = 0;
      trigger = 0;
      dma.enabledMask &= ~(1 << id);
    }
  }
}


auto DMA::Channel::transferChain() -> void {
  dma.debugger.transfer(id);

  u32 address = this->address & ~3;
  u32 startingAddress = address;
  u32 clocks = 0;

  u32 clocksPerWord = 0;
  switch(id) {
    case 0: clocksPerWord = bus.calcAccessTime<false,true>(startingAddress, 4)
                          + bus.calcAccessTime<true,true>(0x1f801820, 4); break;
    case 2: clocksPerWord = bus.calcAccessTime<false,true>(startingAddress, 4)
                          + bus.calcAccessTime<true,true>(0x1f801810, 4); break;
    case 4: clocksPerWord = bus.calcAccessTime<false,true>(startingAddress, 4)
                          + bus.calcAccessTime<true,true>(0x1f801c00, 4); break;
  }

  do {
    n32 data = cpu.ram.readWord(address & 0xfffffc);
    address += 4;

    if(!chain.length) {
      chain.address = data >>  0;
      chain.length  = data >> 24;
    } else if(chain.length--) {
      if(likely(id == 2)) {
        gpu.writeDMA(data);
      } else {
        switch(id) {
        case 0: mdec.writeDMA(data); break;
        case 1: debug(unimplemented, "DMA MDECout chain"); break;
        case 3: debug(unimplemented, "DMA CDROM chain"); break;
        case 4: spu.writeDMA(data); break;
        case 5: debug(unimplemented, "DMA PIO chain"); break;
        case 6: debug(unimplemented, "DMA OTC chain"); break;
        }
      }
    }

    clocks += clocksPerWord;

    if(!chain.length) { address = chain.address; break; }
  } while(true);

  dma.counter = clocks;
  dma.step(clocks);

  this->address = address;
  if(address & 0x800000) {
    enable = 0;
    trigger = 0;
    dma.enabledMask &= ~(1 << id);
  }
}
