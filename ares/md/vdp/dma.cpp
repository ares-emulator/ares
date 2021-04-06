auto VDP::DMA::run() -> void {
  if(!enable || wait) return;
  if(!vdp.io.command.bit(5)) return;
  if(mode <= 1) return load();
  if(mode == 2) return fill();
  if(!vdp.io.command.bit(4)) return;
  if(mode == 3) return copy();
}

auto VDP::DMA::load() -> void {
  active = 1;
  bus.acquire(Bus::VDPDMA);

  auto address = mode.bit(0) << 23 | source << 1;
  auto data = bus.read(1, 1, address);
  vdp.writeDataPort(data);

  source.bit(0,15)++;
  if(--length == 0) {
    vdp.io.command.bit(5) = 0;
    active = 0;
    bus.release(Bus::VDPDMA);
  }
}

auto VDP::DMA::fill() -> void {
  vdp.fifo.write(vdp.io.command, vdp.io.address, filldata);

  source.bit(0,15)++;
  vdp.io.address += vdp.io.dataIncrement;
  if(--length == 0) {
    vdp.io.command.bit(5) = 0;
  }
}

auto VDP::DMA::copy() -> void {
  auto data = vdp.vram.readByte(source);
  vdp.fifo.write(1, vdp.io.address, data);  //VRAM only

  source.bit(0,15)++;
  vdp.io.address += vdp.io.dataIncrement;
  if(--length == 0) {
    vdp.io.command.bit(5) = 0;
  }
}

auto VDP::DMA::power(bool reset) -> void {
  active = 0;
  mode = 0;
  source = 0;
  length = 0;
  filldata = 0;
  enable = 0;
  wait = 0;
}
