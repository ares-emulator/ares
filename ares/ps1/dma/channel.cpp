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

auto DMA::Channel::step(u32 clocks) -> bool {
  if(!masterEnable || !counter) return false;

  if(counter > clocks) {
    counter -= clocks;
    return state == Running;
  }
  counter = 0;

  if(state == Waiting) {
    if((id != OTC && (enable || trigger))
    || (id == OTC && (enable && trigger))
    ) {
      enable = 1;
      trigger = 0;
      state = Running;
      do {
        switch(synchronization) {
        case 0: transferBlock(); break;
        case 1: transferBlock(); break;
        case 2: transferChain(); break;
        }
      } while(id != SPU && synchronization == 1 && enable);
    }
    return false;
  }

  if(synchronization == 0 && enable) {
    state = Waiting;
    counter = 1 << chopping.cpuWindow;
    return false;
  }

  if(synchronization == 1 && enable) {
    state = Waiting;
    counter = 192;
    return false;
  }

  if(synchronization == 2 && enable) {
    state = Waiting;
    counter = 192;
    return false;
  }

  state = Waiting;
  if(irq.enable) {
    irq.flag = 1;
    dma.irq.poll();
  }
  return false;
}

auto DMA::Channel::transferBlock() -> void {
  dma.debugger.transfer(id);

  u32 address = this->address & ~3;
  u16 length  = this->length;
  u32 timeout = synchronization == 0 && chopping.enable ? 1 << chopping.dmaWindow : ~0;
  s16 step    = this->decrement ? -4 : +4;
  u32 clocks  = 0;

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
    clocks  += 1;
  } while(--length && --timeout);

  if(synchronization == 0 && chopping.enable == 0) {
    enable = 0;
  }

  if(synchronization == 0 && chopping.enable == 1) {
    this->address = address;
    this->length  = length;
    if(length == 0) enable = 0;
  }

  if(synchronization == 1) {
    this->address = address;
    this->blocks--;
    if(blocks == 0) enable = 0;
  }

  this->counter = 1 + clocks;
}

auto DMA::Channel::transferChain() -> void {
  dma.debugger.transfer(id);

  u32 address = this->address & ~3;
  u32 clocks  = 0;
  u32 timeout = 0x1000;

  do {
    n32 data = bus.read<Word>(address & 0xfffffc);
    address += 4;
    clocks  += 1;

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
      if(clocks >= timeout) break;
    }
  } while(true);

  this->counter = 1 + clocks;
  this->address = address;
  if(address & 0x800000) enable = 0;
}
