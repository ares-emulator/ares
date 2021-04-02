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

  auto address = io.mode.bit(0) << 23 | io.source << 1;
  auto data = cpu.read(1, 1, address);
  vdp.writeDataPort(data);

  io.source.bit(0,15)++;
  if(--io.length == 0) {
    vdp.io.command.bit(5) = 0;
    active = 0;
  }
}

auto VDP::DMA::fill() -> void {
  switch(vdp.io.command.bit(0,3)) {
  case 1: vdp.vram .writeByte(vdp.io.address, io.fill); break;
  case 5: vdp.vsram.writeByte(vdp.io.address, io.fill); break;
  case 3: vdp.cram .writeByte(vdp.io.address, io.fill); break;
  default:
    debug(unusual, "[VDP] DMA::fill: io.command = 0b", binary(vdp.io.command, 6L));
    break;
  }

  io.source.bit(0,15)++;
  vdp.io.address += vdp.io.dataIncrement;
  if(--io.length == 0) {
    vdp.io.command.bit(5) = 0;
  }
}

//note: this can only copy to VRAM
auto VDP::DMA::copy() -> void {
  auto data = vdp.vram.readByte(io.source);
  vdp.vram.writeByte(vdp.io.address, data);

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
