auto VDP::DMA::poll() -> void {
  static bool locked = false;
  if(locked) return;
  locked = true;
  if(cpu.active()) cpu.synchronize(apu, vdp);
  if(apu.active()) apu.synchronize(cpu, vdp);
  while(run());
  locked = false;
}

auto VDP::DMA::run() -> bool {
  if(vdp.command.pending && !io.wait) {
    if(io.mode <= 1) return load(), true;
    if(io.mode == 2) return fill(), true;
    if(io.mode == 3) return copy(), true;
  }
  return false;
}

auto VDP::DMA::load() -> void {
  active = 1;

  auto address = io.mode.bit(0) << 23 | io.source << 1;
  auto data = bus.read(1, 1, address);
  vdp.writeDataPort(data);

  io.source.bit(0,15)++;
  if(--io.length == 0) {
    vdp.command.pending = 0;
    active = 0;
  }
}

auto VDP::DMA::fill() -> void {
  switch(vdp.command.target) {
  case 1: vdp.vram .writeByte(vdp.command.address, io.fill); break;
  case 5: vdp.vsram.writeByte(vdp.command.address, io.fill); break;
  case 3: vdp.cram .writeByte(vdp.command.address, io.fill); break;
  default:
    debug(unusual, "[VDP::DMA::fill]: command.target = 0x", hex(vdp.command.target));
    break;
  }

  io.source.bit(0,15)++;
  vdp.command.address += vdp.command.increment;
  if(--io.length == 0) {
    vdp.command.pending = 0;
  }
}

//note: this can only copy to VRAM
auto VDP::DMA::copy() -> void {
  auto data = vdp.vram.readByte(io.source);
  vdp.vram.writeByte(vdp.command.address, data);

  io.source.bit(0,15)++;
  vdp.command.address += vdp.command.increment;
  if(--io.length == 0) {
    vdp.command.pending = 0;
  }
}

auto VDP::DMA::power() -> void {
  active = 0;
  io = {};
}
