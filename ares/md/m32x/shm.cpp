auto M32X::SHM::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("SHM");
  bootROM.allocate(2_KiB >> 1);
  if(auto fp = system.pak->read("sh2.boot.mrom")) {
    for(auto address : range(2_KiB >> 1)) bootROM.program(address, fp->readm(2L));
  }
  debugger.load(node);
}

auto M32X::SHM::unload() -> void {
  debugger = {};
  bootROM.reset();
  node.reset();
}

auto M32X::SHM::main() -> void {
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

  debugger.instruction();
  instruction();
  step(1);
}

auto M32X::SHM::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto M32X::SHM::exception() -> bool {
  if(irq.raised) return irq.raised = 0, true;
  return false;
}

auto M32X::SHM::power(bool reset) -> void {
  Thread::create(23'000'000, {&M32X::SHM::main, this});
  SH2::power();
  SH2::PC    = (bootROM[0] << 16 | bootROM[1] << 0) + 4;
  SH2::R[15] = (bootROM[2] << 16 | bootROM[3] << 0);
  irq = {};
}

auto M32X::SHM::readByte(u32 address) -> u32 {
  if(address & 1) {
    return m32x.readInternal(0, 1, address & ~1).byte(0);
  } else {
    return m32x.readInternal(1, 0, address & ~1).byte(1);
  }
}

auto M32X::SHM::readWord(u32 address) -> u32 {
  return m32x.readInternal(1, 1, address & ~1);
}

auto M32X::SHM::readLong(u32 address) -> u32 {
  u32    data = m32x.readInternal(1, 1, address & ~3 | 0) << 16;
  return data | m32x.readInternal(1, 1, address & ~3 | 2) <<  0;
}

auto M32X::SHM::writeByte(u32 address, u32 data) -> void {
  if(address & 1) {
    m32x.writeInternal(0, 1, address & ~1, data << 8 | (u8)data << 0);
  } else {
    m32x.writeInternal(1, 0, address & ~1, data << 8 | (u8)data << 0);
  }
}

auto M32X::SHM::writeWord(u32 address, u32 data) -> void {
  m32x.writeInternal(1, 1, address & ~1, data);
}

auto M32X::SHM::writeLong(u32 address, u32 data) -> void {
  m32x.writeInternal(1, 1, address & ~3 | 0, data >> 16);
  m32x.writeInternal(1, 1, address & ~3 | 2, data >>  0);
}
