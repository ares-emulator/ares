//test if the 68K bus should be acquired immediately for 68K->VDP DMA
auto VDP::DMA::synchronize() -> void {
  if(vdp.command.pending && !wait && mode <= 1) {
    active = 1;
    bus.acquire(Bus::VDPDMA);
  } else {
    active = 0;
    bus.release(Bus::VDPDMA);
  }
}

auto VDP::DMA::run() -> bool {
  if(vdp.command.pending && !wait) {
    if(mode <= 1 && !vdp.fifo.full()) {
      return load(), true;
    } else if(mode == 2 && vdp.fifo.empty()) {
      return fill(), true;
    } else if(mode == 3) {
      return copy(), true;
    }
  }
  return false;
}

auto VDP::DMA::load() -> void {
  auto address = mode.bit(0) << 23 | source << 1;
  auto data = bus.read(1, 1, address);
  vdp.writeDataPort(data);
  vdp.debugger.dmaLoad(address, vdp.command.target, vdp.command.address, data);

  source.bit(0,15)++;
  if(--length == 0) {
    vdp.command.pending = 0;
    synchronize();
  }
}

auto VDP::DMA::fill() -> void {
  switch(vdp.command.target) {
  case 1: vdp.vram.writeByte(vdp.command.address ^ 1, data >> 8); break;
  case 3: vdp.cram.write(vdp.command.address >> 1, data); break;
  case 5: vdp.vsram.write(vdp.command.address >> 1, data); break;
  }
  vdp.debugger.dmaFill(vdp.command.target, vdp.command.address, data);

  source.bit(0,15)++;
  vdp.command.address += vdp.command.increment;
  if(--length == 0) {
    vdp.command.pending = 0;
    synchronize();
  }
}

//VRAM only
auto VDP::DMA::copy() -> void {
  if(!read) {
    read = 1;
    data = vdp.vram.readByte(source ^ 1);
    return;
  }

  read = 0;
  vdp.vram.writeByte(vdp.command.address ^ 1, data);
  vdp.debugger.dmaCopy(source, vdp.command.target, vdp.command.address ^ 1, data);

  source.bit(0,15)++;
  vdp.command.address += vdp.command.increment;
  if(--length == 0) {
    vdp.command.pending = 0;
    synchronize();
  }
}

auto VDP::DMA::power(bool reset) -> void {
  active = 0;
  mode = 0;
  source = 0;
  length = 0;
  wait = 1;
  read = 0;
  enable = 0;
}
