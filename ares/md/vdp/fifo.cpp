auto VDP::FIFO::advance() -> void {
  swap(slots[0], slots[1]);
  swap(slots[1], slots[2]);
  swap(slots[2], slots[3]);
}

auto VDP::FIFO::run() -> bool {
  if(empty()) return false;

  if(slots[0].target == 1 && vdp.vram.mode == 0) {
    if(slots[0].lower) {
      slots[0].lower = 0;
      vdp.vram.writeByte(slots[0].address & ~1 | 1, slots[0].data.byte(0));
      return true;
    }
    if(slots[0].upper) {
      slots[0].upper = 0;
      vdp.vram.writeByte(slots[0].address & ~1 | 0, slots[0].data.byte(1));
      return advance(), true;
    }
  }

  if(slots[0].target == 1 && vdp.vram.mode == 1) {
    slots[0].lower = 0;
    slots[0].upper = 0;
    vdp.vram.writeByte(slots[0].address | 1, slots[0].data.byte(0));
    return advance(), true;
  }

  if(slots[0].target == 3) {
    slots[0].lower = 0;
    slots[0].upper = 0;
    vdp.cram.write(slots[0].address >> 1, slots[0].data);
    return advance(), true;
  }

  if(slots[0].target == 5) {
    slots[0].lower = 0;
    slots[0].upper = 0;
    vdp.vsram.write(slots[0].address >> 1, slots[0].data);
    return advance(), true;
  }

  slots[0].lower = 0;
  slots[0].upper = 0;
  debug(unusual, "[VDP::FIFO] write target = 0x", hex(slots[0].target));
  cpu.debugger.interrupt({"VDP FIFO ", hex(slots[0].target)});
  return advance(), true;
}

auto VDP::FIFO::write(n4 target, n17 address, n16 data) -> void {
  if(target.bit(0) != 1) return;
  if(full()) {
    bus.acquire(Bus::VDPFIFO);
    while(full()) {
      if(cpu.active()) cpu.wait(1);
      if(apu.active()) apu.step(1);
    }
    bus.release(Bus::VDPFIFO);
  }

  if(address.bit(0) && target == 1) {
    data = data << 8 | data >> 8;
  }

  for(auto& slot : slots) {
    if(slot.empty()) {
      slot.target  = target;
      slot.address = address;
      slot.data    = data;
      slot.upper   = 1;
      slot.lower   = 1;
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
