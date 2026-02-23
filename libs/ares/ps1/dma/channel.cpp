auto DMA::sortChannelsByPriority() -> void {
  n1 selected[7];
  for(u32 index : range(7)) {
    u32 id = 8, lowest = 8;
    for(u32 search : range(7)) {
      if(selected[search]) continue;
      if(channels[search].priority < lowest) {
        lowest = channels[search].priority;
        id = search;
      }
    }
    selected[id] = 1;
    channelsByPriority[index] = id;
  }
}

auto DMA::Channel::kick() -> bool {
  if(!masterEnable) return false;
  if(!enable) return false;

  if(state == Idle) {
    static bool (* const canRead[7])() = {
      []() { return false; },
      []() { return mdec.canReadDMA(); },
      []() { return gpu.canReadDMA(); },
      []() { return disc.canReadDMA(); },
      []() { return spu.canReadDMA(); },
      []() { return false; },
      []() { return false; }
    };

    static bool (* const canWrite[7])() = {
      []() { return mdec.canWriteDMA(); },
      []() { return false; },
      []() { return gpu.canWriteDMA(); },
      []() { return false; },
      []() { return spu.canWriteDMA(); },
      []() { return false; },
      []() { return false; }
    };

    auto dmaReq = trigger;
    if(direction == 0 && canRead[id]()) dmaReq = true;
    if(direction == 1 && canWrite[id]()) dmaReq = true;
    if(!dmaReq) return false;

    trigger = 0;
    state = Running;
    return true;
  }

  return false;
}

auto DMA::Channel::step() -> bool {
  if(!masterEnable) return false;
  if(!enable) return false;

  if(state == Idle) {
    static bool (* const canRead[7])() = {
      []() { return false; },
      []() { return mdec.canReadDMA(); },
      []() { return gpu.canReadDMA(); },
      []() { return disc.canReadDMA(); },
      []() { return spu.canReadDMA(); },
      []() { return false; },
      []() { return false; }
    };

    static bool (* const canWrite[7])() = {
      []() { return mdec.canWriteDMA(); },
      []() { return false; },
      []() { return gpu.canWriteDMA(); },
      []() { return false; },
      []() { return spu.canWriteDMA(); },
      []() { return false; },
      []() { return false; }
    };

    auto dmaReq = trigger;
    if(direction == 0 && canRead[id]()) dmaReq = true;
    if(direction == 1 && canWrite[id]()) dmaReq = true;
    if(!dmaReq) return false;

    trigger = 0;
    state = Running;
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

  do {
    //DMA -> CPU
    if(direction == 0) {
      u32 data = 0;
      switch(id) {
      case 0: debug(unimplemented, "DMA MDECin read"); break;
      case 1: data = mdec.readDMA(); break;
      case 2: data = gpu.readDMA(); break;
      case 3: data = disc.readDMA(); break;
      case 4: data = spu.readDMA(); break;
      case 5: debug(unimplemented, "DMA PIO read"); break;
      case 6: data = address - 4 & 0xfffffc; if(length == 1) data = 0xffffff; break;
      }
      bus.write<Word>(address, data);
    }

    //CPU -> DMA
    if(direction == 1) {
      u32 data = bus.read<Word>(address);
      switch(id) {
      case 0: mdec.writeDMA(data); break;
      case 1: debug(unimplemented, "DMA MDECout write"); break;
      case 2: gpu.writeDMA(data); break;
      case 3: debug(unimplemented, "DMA CDROM write"); break;
      case 4: spu.writeDMA(data); break;
      case 5: debug(unimplemented, "DMA PIO write"); break;
      case 6: debug(unimplemented, "DMA OTC write"); break;
      }
    }

    address += step;
    wordsTransferred++;
  } while(--length && --timeout);

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
      clocks += bus.calcAccessTime<false,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<true,true>(0x1f801810, bytes);
      break;
    case 4: // SPU
      clocks += bus.calcAccessTime<false,true>(startingAddress, bytes);
      clocks += bus.calcAccessTime<true,true>(0x1f801c00, bytes);
      break;
    }
  }

  dma.counter = wordsTransferred;
  dma.step(wordsTransferred);

  if(synchronization == 0) {
    if(chopping.enable == 1) {
      this->length = length;
      this->address = address;
    }

    if(length == 0) enable = 0;
    return;
  }

  this->address = address;
  if(length == 0) {
    this->blocks--;
    if(blocks == 0) enable = 0;
  }
}

auto DMA::Channel::transferChain() -> void {
  dma.debugger.transfer(id);

  u32 address = this->address & ~3;
  u32 startingAddress = address;
  u32 wordsTransferred  = 0;
  u32 timeout = 0x1000;

  do {
    n32 data = bus.read<Word>(address & 0xfffffc);
    address += 4;
    wordsTransferred  += 1;

    if(!chain.length) {
      chain.address = data >>  0;
      chain.length  = data >> 24;
    } else if(chain.length--) {
      switch(id) {
      case 0: mdec.writeDMA(data); break;
      case 1: debug(unimplemented, "DMA MDECout chain"); break;
      case 2: gpu.writeDMA(data); break;
      case 3: debug(unimplemented, "DMA CDROM chain"); break;
      case 4: spu.writeDMA(data); break;
      case 5: debug(unimplemented, "DMA PIO chain"); break;
      case 6: debug(unimplemented, "DMA OTC chain"); break;
      }
    }
    if(!chain.length) {
      address = chain.address;
      if(address & 1 << 23) break;
      if(wordsTransferred >= timeout) break;
    }

    u32 clocks = 0;
    switch(id) {
      case 0: // MDECin
        clocks += bus.calcAccessTime<false,true>(startingAddress, 4);
        clocks += bus.calcAccessTime<true,true>(0x1f801820, 4);
        break;
      case 2: // GPU
        clocks += bus.calcAccessTime<false,true>(startingAddress, 4);
        clocks += bus.calcAccessTime<true,true>(0x1f801810, 4);
        break;
      case 4: // SPU
        clocks += bus.calcAccessTime<false,true>(startingAddress, 4);
        clocks += bus.calcAccessTime<true,true>(0x1f801c00, 4);
        break;
    }

    dma.counter = clocks;
    dma.step(dma.counter);
  } while(true);

  this->address = address;
  if(address & 0x800000) enable = 0;
}
