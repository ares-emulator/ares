inline auto CPU::DMA::run() -> bool {
  if(!active || waiting) return false;

  transfer();
  if(irq) cpu.setInterruptFlag(CPU::Interrupt::DMA0 << id);
  if(drq && id == 3) cpu.setInterruptFlag(CPU::Interrupt::Cartridge);
  return true;
}

auto CPU::DMA::transfer() -> void {
  u32 seek = size ? 4 : 2;
  u32 mode = size ? Word : Half;

  if(!cpu.context.dmaRan) {
    cpu.context.dmaRan = true;
    cpu.idle();
  }

  if(cpu.context.dmaActiveChannel != id) {
    //channel has switched - new burst transfer must be started
    cpu.context.dmaRomAccess = 0;
    cpu.context.dmaActiveChannel = id;
  }

  if(latch.source() < 0x0200'0000) {
    cpu.idle();  //cannot access BIOS
  } else {
    n32 addr = latch.source();
    if(mode & Word) addr &= ~3;
    if(mode & Half) addr &= ~1;
    u32 sequential = Nonsequential;
    if(cpu.context.dmaRomAccess) sequential = Sequential;
    latch.data = cpu.get(mode | sequential, addr);
    if(mode & Half) latch.data |= latch.data << 16;
  }

  if(latch.target() < 0x0200'0000) {
    cpu.idle();  //cannot access BIOS
  } else {
    n32 addr = latch.target();
    if(mode & Word) addr &= ~3;
    if(mode & Half) addr &= ~1;
    u32 sequential = Nonsequential;
    if(cpu.context.dmaRomAccess) sequential = Sequential;
    cpu.set(mode | sequential, addr, latch.data >> (addr & 2) * 8);
  }

  switch(sourceMode) {
  case 0: latch.source.data += seek; break;
  case 1: latch.source.data -= seek; break;
  }

  switch(targetMode) {
  case 0: latch.target.data += seek; break;
  case 1: latch.target.data -= seek; break;
  case 3: latch.target.data += seek; break;
  }

  latch.length.data--;
  if(!latch.length()) {
    active = false;
    if(targetMode == 3) latch.target = target;
    if(repeat == 1) latch.length = length;
    if(repeat == 0) enable = false;
  }
}

auto CPU::dmaVblank() -> void {
  for(auto& dma : this->dma) {
    if(dma.enable && dma.timingMode == 1) dma.active = true;
  }
}

auto CPU::dmaHblank() -> void {
  for(auto& dma : this->dma) {
    if(dma.enable && dma.timingMode == 2) dma.active = true;
  }
}

auto CPU::dmaHDMA() -> void {
  auto& dma = this->dma[3];
  if(dma.enable && dma.timingMode == 3) dma.active = true;
}
