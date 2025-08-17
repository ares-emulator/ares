//returns true if DMA is currently active
auto CPU::DMAC::step() -> bool {
  if(!active) {
    //DMA cannot start if bus is locked
    if(cpu.context.busLocked) return false;

    //check if DMA is ready
    if(channel[0].ready() || channel[1].ready() || channel[2].ready() || channel[3].ready()) {
      //transfer control of bus from CPU to DMA
      active = true;
      cpu.ARM7TDMI::nonsequential = true;
      cpu.prefetchStep(1);
      return true;
    }
    return false;
  }

  //run write transaction, if read has finished prior
  if(writeCycle) { channel[activeChannel].write(); return true; }

  //run read transaction
  if(channel[0].ready()) { channel[0].read(); return true; }
  if(channel[1].ready()) { channel[1].read(); return true; }
  if(channel[2].ready()) { channel[2].read(); return true; }
  if(channel[3].ready()) { channel[3].read(); return true; }

  //DMA has finished - hand bus control back to CPU
  cpu.prefetchStep(1);
  romBurst = false;
  activeChannel = 0;  //assign burst to a channel that cannot access ROM
  active = false;
  return false;
}

auto CPU::DMAC::runPending() -> void {
  stallingCPU = true;
  while(step());
  stallingCPU = false;
}

inline auto CPU::DMAC::Channel::ready() -> bool {
  return active && !waiting;
}

auto CPU::DMAC::Channel::read() -> void {
  u32 seek = size ? 4 : 2;
  u32 mode = DMA | (size ? Word : Half );

  if(cpu.dmac.activeChannel != id) {
    //channel has switched - new burst transfer must be started
    cpu.dmac.romBurst = 0;
    cpu.dmac.activeChannel = id;
  }

  if(latch.source() < 0x0200'0000) {
    cpu.prefetchStep(1);  //cannot access BIOS
  } else {
    n32 addr = latch.source();
    if(mode & Word) addr &= ~3;
    if(mode & Half) addr &= ~1;
    latch.data = cpu.get(mode, addr);
    if(mode & Half) latch.data |= latch.data << 16;
  }

  switch(sourceMode) {
  case 0: latch.source.data += seek; break;
  case 1: latch.source.data -= seek; break;
  }

  cpu.dmac.writeCycle = true;
}

auto CPU::DMAC::Channel::write() -> void {
  u32 seek = size ? 4 : 2;
  u32 mode = DMA | (size ? Word : Half );

  if(latch.target() < 0x0200'0000) {
    cpu.prefetchStep(1);  //cannot access BIOS
  } else {
    n32 addr = latch.target();
    if(mode & Word) addr &= ~3;
    if(mode & Half) addr &= ~1;
    cpu.set(mode, addr, latch.data >> (addr & 2) * 8);
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

  cpu.dmac.writeCycle = false;
  if(irq) cpu.setInterruptFlag(CPU::Interrupt::DMA0 << id);
  if(drq && id == 3) cpu.setInterruptFlag(CPU::Interrupt::Cartridge);
}

auto CPU::dmaVblank() -> void {
  for(auto& channel : this->dmac.channel) {
    if(channel.enable && channel.timingMode == 1) channel.active = true;
  }
}

auto CPU::dmaHblank() -> void {
  for(auto& channel : this->dmac.channel) {
    if(channel.enable && channel.timingMode == 2) channel.active = true;
  }
}

auto CPU::dmaHDMA() -> void {
  auto& channel = this->dmac.channel[3];
  if(channel.enable && channel.timingMode == 3) channel.active = true;
}
