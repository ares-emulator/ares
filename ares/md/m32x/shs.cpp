auto M32X::SHS::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("SHS");
  bootROM.allocate(1_KiB >> 1);
  if(auto fp = system.pak->read("sh2.boot.srom")) {
    for(auto address : range(1_KiB >> 1)) bootROM.program(address, fp->readm(2L));
  }
  debugger.load(node);
}

auto M32X::SHS::unload() -> void {
  debugger = {};
  bootROM.reset();
  node.reset();
}

auto M32X::SHS::main() -> void {
  if(irq.vres.active && irq.vres.enable) {
    debugger.interrupt("VRES");
    irq.vres.active = 0;
    return irq.raised = 1, interrupt(14, 71);
  }
  if(irq.vint.active && irq.vint.enable && SR.I < 12) {
    debugger.interrupt("VINT");
    return irq.raised = 1, interrupt(12, 70);
  }
  if(irq.hint.active && irq.hint.enable && SR.I < 10) {
    debugger.interrupt("HINT");
    return irq.raised = 1, interrupt(10, 69);
  }
  if(irq.cmd.active && irq.cmd.enable && SR.I < 8) {
    debugger.interrupt("CMD");
    return irq.raised = 1, interrupt(8, 68);
  }
  if(irq.pwm.active && irq.pwm.enable && SR.I < 6) {
    debugger.interrupt("PWM");
    return irq.raised = 1, interrupt(6, 67);
  }

  for(u32 n : range(2)) {
    if(dmac[n].chcr.de && !m32x.dreq.fifo.empty()) {
      u32 data = 0;
      data |= m32x.dreq.fifo.read(0) << 16;
      data |= m32x.dreq.fifo.read(0) <<  0;
      busWriteLong(dmac[n].dar, data);
      dmac[n].dar += 4;
      dmac[n].tcr -= 1;
      if(!dmac[n].tcr) {
        dmac[n].chcr.de = 0;
        dmac[n].chcr.te = 1;
      }
    }
  }

  debugger.instruction();
  instruction();
  step(1);
}

auto M32X::SHS::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto M32X::SHS::exception() -> bool {
  if(irq.raised) return irq.raised = 1, true;
  return false;
}

auto M32X::SHS::power(bool reset) -> void {
  Thread::create(23'000'000, {&M32X::SHS::main, this});
  SH2::power();
  SH2::PC    = (bootROM[0] << 16 | bootROM[1] << 0) + 4;
  SH2::R[15] = (bootROM[2] << 16 | bootROM[3] << 0);
  irq = {};
  irq.vres.enable = 1;
}

auto M32X::SHS::busReadByte(u32 address) -> u32 {
  if(address & 1) {
    return m32x.readInternal(0, 1, address & ~1).byte(0);
  } else {
    return m32x.readInternal(1, 0, address & ~1).byte(1);
  }
}

auto M32X::SHS::busReadWord(u32 address) -> u32 {
  return m32x.readInternal(1, 1, address & ~1);
}

auto M32X::SHS::busReadLong(u32 address) -> u32 {
  u32    data = m32x.readInternal(1, 1, address & ~3 | 0) << 16;
  return data | m32x.readInternal(1, 1, address & ~3 | 2) <<  0;
}

auto M32X::SHS::busWriteByte(u32 address, u32 data) -> void {
  if(address & 1) {
    m32x.writeInternal(0, 1, address & ~1, data << 8 | (u8)data << 0);
  } else {
    m32x.writeInternal(1, 0, address & ~1, data << 8 | (u8)data << 0);
  }
}

auto M32X::SHS::busWriteWord(u32 address, u32 data) -> void {
  m32x.writeInternal(1, 1, address & ~1, data);
}

auto M32X::SHS::busWriteLong(u32 address, u32 data) -> void {
  m32x.writeInternal(1, 1, address & ~3 | 0, data >> 16);
  m32x.writeInternal(1, 1, address & ~3 | 2, data >>  0);
}
