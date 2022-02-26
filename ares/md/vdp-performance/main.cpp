auto VDP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}

auto VDP::vtick() -> void {
  if(vblank()) {
    irq.hblank.counter = irq.hblank.frequency;
  } else if(irq.hblank.counter-- == 0) {
    irq.hblank.counter = irq.hblank.frequency;
    irq.hblank.pending = 1;
    irq.poll();
  }

  state.vcounter++;
  if(v28()) {
    if(vcounter() == 0x0e0) vblank(1);
    if(vcounter() == 0x0eb && Region::NTSC()) state.vcounter = 0x1e5;
    if(vcounter() == 0x103 && Region::PAL ()) state.vcounter = 0x1ca;
    if(vcounter() == 0x1ff) vblank(0);
  }
  if(v30()) {
    if(vcounter() == 0x0f0) vblank(1);
    if(vcounter() == 0x200 && Region::NTSC()) state.vcounter = 0x000;
    if(vcounter() == 0x10b && Region::PAL ()) state.vcounter = 0x1d2;
    if(vcounter() == 0x1ff) vblank(0);
  }
}

auto VDP::hblank(bool line) -> void {
  state.hblank = line;
  if(hblank() == 0) {
    cartridge.hblank(0);
  } else {
    cartridge.hblank(1);
    apu.setINT(0);  //timing hack
  }
}

auto VDP::vblank(bool line) -> void {
  state.vblank = line;
  irq.vblank.transitioned = 1;
}

auto VDP::vedge() -> void {
  if(!irq.vblank.transitioned) return;
  irq.vblank.transitioned = 0;

  if(vblank() == 0) {
    cartridge.vblank(0);
  //apu.setINT(0);
  } else {
    cartridge.vblank(1);
    apu.setINT(1);
    irq.vblank.pending = 1;
    irq.poll();
  }
}

auto VDP::main() -> void {
  if(hcounter() == 0) {
    latch.displayWidth = io.displayWidth;
    latch.clockSelect  = io.clockSelect;

    step(512);
    state.hcounter = 0x80;
  } else if(hcounter() == 0x80) {
    if(vcounter() < screenHeight() && !runAhead()) {
      render();
      m32x.vdp.scanline(pixels(), state.vcounter);
    }

    step(768);
    state.hcounter = h32() ? 0xe9 : 0xe4;
    hblank(1);
    vtick();
  } else {
    step(430);
    state.hcounter = 0;
    hblank(0);
    vedge();

    if(vcounter() == 0) {
      latch.interlace = io.interlaceMode == 3;
      latch.overscan  = io.overscan;
      frame();
      state.field ^= 1;
    }
  }
}
