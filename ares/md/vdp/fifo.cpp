auto VDP::FIFO::advance() -> void {
  slots[0].upper = 0;
  slots[0].lower = 0;
  swap(slots[0], slots[1]);
  swap(slots[1], slots[2]);
  swap(slots[2], slots[3]);
}

auto VDP::FIFO::slot() -> void {
  if(!cache.full()) {
    if(vdp.command.target == 0) {
      if(!cache.lower) {
        cache.lower = 1;
        cache.data.byte(0) = vdp.vram.readByte(vdp.command.address & ~1 | 1);
        vdp.command.ready = 1;
        return;
      }
      if(!cache.upper) {
        cache.upper = 1;
        cache.data.byte(1) = vdp.vram.readByte(vdp.command.address & ~1 | 0);
        vdp.command.ready = 1;
        return;
      }
    }

    if(vdp.command.target == 4) {
      cache.lower = 1;
      cache.upper = 1;
      cache.data = vdp.vsram.read(vdp.command.address >> 1);
      vdp.command.ready = 1;
      return;
    }

    if(vdp.command.target == 8) {
      cache.lower = 1;
      cache.upper = 1;
      cache.data = vdp.cram.read(vdp.command.address >> 1);
      vdp.command.ready = 1;
      return;
    }

    cache.lower = 1;
    cache.upper = 1;
    vdp.command.ready = 1;
    debug(unusual, "[VDP::FIFO] read target = 0x", hex(vdp.command.target));
    return;
  }

  if(!slots[0].empty()) {
    if(slots[0].target == 1) {
      if(slots[0].lower) {
        slots[0].lower = 0;
        vdp.vram.writeByte(slots[0].address << 1 | 1, slots[0].data.byte(0));
        return;
      }
      if(slots[0].upper) {
        slots[0].upper = 0;
        vdp.vram.writeByte(slots[0].address << 1 | 0, slots[0].data.byte(1));
        return advance();
      }
    }

    if(slots[0].target == 3) {
      vdp.cram.write(slots[0].address, slots[0].data);
      return advance();
    }

    if(slots[0].target == 5) {
      vdp.vsram.write(slots[0].address, slots[0].data);
      return advance();
    }

    slots[0].lower = 0;
    slots[0].upper = 0;
    debug(unusual, "[VDP::FIFO] write target = 0x", hex(slots[0].target));
    return advance();
  }

  vdp.dma.run();
}

auto VDP::FIFO::read(n4 target, n17 address) -> void {
  cache.upper = address.bit(0) && target == 0;
  cache.lower = 0;
}

auto VDP::FIFO::write(n4 target, n17 address, n16 data) -> void {
  if(full()) {
    bus.acquire(Bus::VDPFIFO);
    while(full()) cpu.wait(1);
    bus.release(Bus::VDPFIFO);
  }

  if(address.bit(0) && target == 1) {
    data = data << 8 | data >> 8;
  }

  for(auto& slot : slots) {
    if(slot.empty()) {
      slot.target  = target;
      slot.address = address >> 1;
      slot.data    = data;
      slot.upper   = 1;
      slot.lower   = 1;
      return;
    }
  }
}

auto VDP::FIFO::power(bool reset) -> void {
  cache = {};
  cache.upper = 1;
  cache.lower = 1;
  slots[0] = {};
  slots[1] = {};
  slots[2] = {};
  slots[3] = {};
}
