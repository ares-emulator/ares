auto VDP::FIFO::tick() -> void {
  for(auto& slot : slots)
    if(!slot.empty() && slot.latency > 0)
      slot.latency--;
}

auto VDP::FIFO::advance() -> void {
  if(vdp.command.pending && vdp.dma.mode == 2) {
    if(slots[0].target == 1)
      vdp.dma.data = slots[0].data;
    else
      vdp.dma.data = slots[1].data; // fill data taken from next fifo slot (late fetch)
    vdp.dma.read = 1;
    vdp.dma.wait = 0; // start pending DMA if necessary
  }

  swap(slots[0], slots[1]);
  swap(slots[1], slots[2]);
  swap(slots[2], slots[3]);
}

auto VDP::FIFO::run() -> bool {
  if(empty()) return false;
  if(slots[0].latency > 0) return false;
  if(vdp.dma.active && vdp.dma.preload > 0) return false;

  if(slots[0].target == 1 && vdp.vram.mode == 0) {
    if(slots[0].lower) {
      slots[0].lower = 0;
      vdp.vram.writeByte(slots[0].address ^ 1, slots[0].data.byte(0));
      return true;
    }
    if(slots[0].upper) {
      slots[0].upper = 0;
      vdp.vram.writeByte(slots[0].address, slots[0].data.byte(1));
      return advance(), true;
    }
  }

  // If the target is VRAM read, there is assumed to be no effect.
  // However, fifo timing must account for VRAM byte access (as above).
  if(slots[0].target == 0 && vdp.vram.mode == 0) {
    debug(unusual, "[VDP::FIFO] write target = 0 (VRAM read)");
    if(slots[0].lower) {
      slots[0].lower = 0;
      // null action
      return true;
    }
    if(slots[0].upper) {
      slots[0].upper = 0;
      // null action
      return advance(), true;
    }
  }

  if(slots[0].target == 1 && vdp.vram.mode == 1)
    vdp.vram.writeByte(slots[0].address | 1, slots[0].data.byte(0));
  else if(slots[0].target == 3)
    vdp.cram.write(slots[0].address >> 1, slots[0].data);
  else if(slots[0].target == 5)
    vdp.vsram.write(slots[0].address >> 1, slots[0].data);
  else
    debug(unusual, "[VDP::FIFO] write target = 0x", hex(slots[0].target));

  slots[0].lower = 0;
  slots[0].upper = 0;
  return advance(), true;
}

auto VDP::FIFO::write(n4 target, n17 address, n16 data) -> void {
  if(full()) {
    bus.acquire(Bus::VDPFIFO);
    while(full()) {
      if(cpu.active()) cpu.wait(1);
      if(apu.active()) apu.step(1);
    }
    bus.release(Bus::VDPFIFO);
  }

  for(auto& slot : slots) {
    if(slot.empty()) {
      slot.target  = target;
      slot.address = address;
      slot.data    = data;
      slot.upper   = 1;
      slot.lower   = 1;
      slot.latency = 2;
      return;
    }
  }
}

auto VDP::FIFO::power(bool reset) -> void {
  slots[0] = {};
  slots[1] = {};
  slots[2] = {};
  slots[3] = {};
}
