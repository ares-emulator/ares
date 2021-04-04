auto VDP::DMA::run() -> void {
  if(!io.enable || io.wait) return;
  if(!vdp.io.command.bit(5)) return;
  if(io.mode <= 1) return load();
  if(io.mode == 2) return fill();
  if(!vdp.io.command.bit(4)) return;
  if(io.mode == 3) return copy();
}

auto VDP::DMA::load() -> void {
  active = 1;
  bus.acquire(Bus::VDPDMA);

  auto address = io.mode.bit(0) << 23 | io.source << 1;
  auto data = bus.read(1, 1, address);
  vdp.writeDataPort(data);

  io.source.bit(0,15)++;
  if(--io.length == 0) {
    vdp.io.command.bit(5) = 0;
    active = 0;
    bus.release(Bus::VDPDMA);
  }
}

auto VDP::DMA::fill() -> void {
  vdp.fifo.write(vdp.io.address, io.fill, vdp.io.command);

  io.source.bit(0,15)++;
  vdp.io.address += vdp.io.dataIncrement;
  if(--io.length == 0) {
    vdp.io.command.bit(5) = 0;
  }
}

auto VDP::DMA::copy() -> void {
  auto data = vdp.vram.readByte(io.source);
  vdp.fifo.write(vdp.io.address, data, 1);  //VRAM only

  io.source.bit(0,15)++;
  vdp.io.address += vdp.io.dataIncrement;
  if(--io.length == 0) {
    vdp.io.command.bit(5) = 0;
  }
}

auto VDP::DMA::power() -> void {
  active = 0;
  io = {};
}
