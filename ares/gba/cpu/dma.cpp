inline auto CPU::DMA::run() -> bool {
  if(!active || waiting) return false;

  transfer();
  if(irq) cpu.irq.flag |= CPU::Interrupt::DMA0 << id;
  if(drq && id == 3) cpu.irq.flag |= CPU::Interrupt::Cartridge;
  return true;
}

auto CPU::DMA::transfer() -> void {
  u32 seek = size ? 4 : 2;
  u32 mode = size ? Word : Half;
  mode |= latch.length() == length() ? Nonsequential : Sequential;

  if(!cpu.context.dmaRan) {
    cpu.context.dmaRan = true;
    cpu.idle();
  }

  if(latch.source() < 0x0200'0000) {
    cpu.idle();  //cannot access BIOS
  } else {
    n32 addr = latch.source();
    if(mode & Word) addr &= ~3;
    if(mode & Half) addr &= ~1;
    if(addr & 0x0800'0000) cpu.context.dmaRomAccess = true;
    latch.data = cpu.get(mode, addr);
    if(mode & Half) latch.data |= latch.data << 16;
  }

  if(mode & Nonsequential) {
    if((source() & 0x0800'0000) && (target() & 0x0800'0000)) {
      //ROM -> ROM transfer
      mode |= Sequential;
      mode ^= Nonsequential;
    }
  }

  if(latch.target() < 0x0200'0000) {
    cpu.idle();  //cannot access BIOS
  } else {
    n32 addr = latch.target();
    if(mode & Word) addr &= ~3;
    if(mode & Half) addr &= ~1;
    if(addr & 0x0800'0000) cpu.context.dmaRomAccess = true;
    cpu.set(mode, addr, latch.data >> (addr & 2) * 8);
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
