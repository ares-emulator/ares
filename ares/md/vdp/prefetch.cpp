auto VDP::Prefetch::run() -> bool {
  if(full()) return false;

  if(vdp.command.target == 0 && vdp.vram.mode == 0) {
    if(!slot.lower) {
      slot.lower = 1;
      slot.data.byte(0) = vdp.vram.readByte(vdp.command.address & ~1 | 1);
      return true;
    }
    if(!slot.upper) {
      slot.data.byte(1) = vdp.vram.readByte(vdp.command.address & ~1 | 0);
      slot.upper = 1;
      vdp.command.ready = 1;
      return true;
    }
  }

  if(vdp.command.target == 0 && vdp.vram.mode == 1) {
    slot.lower = 1;
    slot.upper = 1;
    slot.data.byte(0) = vdp.vram.readByte(vdp.command.address | 1);
    slot.data.byte(1) = slot.data.byte(0);
    vdp.command.ready = 1;
    return true;
  }

  if(vdp.command.target == 4) {
    slot.lower = 1;
    slot.upper = 1;
    slot.data = vdp.vsram.read(vdp.command.address >> 1);
    slot.data = (slot.data & 0x07FF) | (vdp.fifo.slots[0].data & 0xF800);
    vdp.command.ready = 1;
    return true;
  }

  if(vdp.command.target == 8) {
    slot.lower = 1;
    slot.upper = 1;
    slot.data = vdp.cram.read(vdp.command.address >> 1);
    slot.data = slot.data & 0x0EEE | vdp.fifo.slots[0].data & ~0x0EEE;
    vdp.command.ready = 1;
    return true;
  }

  if(vdp.command.target == 12) {
    slot.lower = 1;
    slot.upper = 1;
    slot.data.byte(0) = vdp.vram.readByte(vdp.command.address ^ 1);
    slot.data.byte(1) = vdp.fifo.slots[0].data.byte(1);
    vdp.command.ready = 1;
    return true;
  }

  slot.lower = 1;
  slot.upper = 1;
  vdp.command.ready = 1;
  debug(unusual, "[VDP::Prefetch] read target = 0x", hex(vdp.command.target));
  return true;
}

auto VDP::Prefetch::read(n4 target, n17 address) -> void {
  if(target.bit(0) != 0) return;
  slot.upper = 0;
  slot.lower = 0;
}

auto VDP::Prefetch::power(bool reset) -> void {
  slot = {};
  slot.upper = 1;
  slot.lower = 1;
}
