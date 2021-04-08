auto VDP::DMA::run() -> void {
  if(enable && !wait && vdp.command.pending) {
    if(mode <= 1) return load();
    if(mode == 2) return fill();
    if(mode == 3) return copy();
  }
}

auto VDP::DMA::load() -> void {
  active = 1;
  bus.acquire(Bus::VDPDMA);

  auto address = mode.bit(0) << 23 | source << 1;
  auto data = bus.read(1, 1, address);
  vdp.writeDataPort(data);

  source.bit(0,15)++;
  if(--length == 0) {
    vdp.command.pending = 0;
    active = 0;
    bus.release(Bus::VDPDMA);
  }
}

auto VDP::DMA::fill() -> void {
  auto data = vdp.fifo.slots[3].data;
  switch(vdp.command.target) {
  case 1: vdp.vram.writeByte(vdp.command.address, data >> 8); break;
  case 3: vdp.cram.write(vdp.command.address, data); break;
  case 5: vdp.vsram.write(vdp.command.address, data); break;
  }

  source.bit(0,15)++;
  vdp.command.address += vdp.command.increment;
  if(--length == 0) {
    vdp.command.pending = 0;
  }
}

//VRAM only
auto VDP::DMA::copy() -> void {
  if(!wait) {
    wait = 1;
    data = vdp.vram.readByte(source);
    return;
  }

  wait = 0;
  vdp.vram.writeByte(vdp.command.address, data);

  source.bit(0,15)++;
  vdp.command.address += vdp.command.increment;
  if(--length == 0) {
    vdp.command.pending = 0;
  }
}

auto VDP::DMA::power(bool reset) -> void {
  active = 0;
  mode = 0;
  source = 0;
  length = 0;
  wait = 0;
  enable = 0;
}
