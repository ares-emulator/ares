auto M32X::SH7604::load(Node::Object parent, string name, string bootFile) -> void {
  node = parent->append<Node::Object>(name);
  if(auto fp = system.pak->read(bootFile)) {
    bootROM.allocate(fp->size() >> 1);
    for(auto address : range(bootROM.size())) bootROM.program(address, fp->readm(2L));
  }
  debugger.load(node);
}

auto M32X::SH7604::unload() -> void {
  debugger = {};
  bootROM.reset();
  node.reset();
}

auto M32X::SH7604::main() -> void {
  if(!SH2::inDelaySlot() && !SH2::ID) {
    if(irq.vres.active && irq.vres.enable) {
      debugger.interrupt("VRES");
      irq.vres.active = 0;
      return ET = 1, interrupt(14, 71);
    }
    if(irq.vint.active && irq.vint.enable && SR.I < 12) {
      debugger.interrupt("VINT");
      return ET = 1, interrupt(12, 70);
    }
    if(irq.hint.active && irq.hint.enable && SR.I < 10) {
      debugger.interrupt("HINT");
      return ET = 1, interrupt(10, 69);
    }
    if(irq.cmd.active && irq.cmd.enable && SR.I < 8) {
      debugger.interrupt("CMD");
      return ET = 1, interrupt(8, 68);
    }
    if(irq.pwm.active && irq.pwm.enable && SR.I < 6) {
      debugger.interrupt("PWM");
      return ET = 1, interrupt(6, 67);
    }
  }

  debugger.instruction();
  SH2::instruction();
  SH2::intc.run();
  SH2::dmac.run();
  SH2::frt.run();
}

auto M32X::SH7604::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto M32X::SH7604::power(bool reset) -> void {
  Thread::create(23'000'000, {&M32X::SH7604::main, this});
  recompiler.min_cycles = 22;
  SH2::power(reset);
  irq = {};
  irq.vres.enable = 1;
}

auto M32X::SH7604::busReadByte(u32 address) -> u32 {
  if(address & 1) {
    return m32x.readInternal(0, 1, address & ~1).byte(0);
  } else {
    return m32x.readInternal(1, 0, address & ~1).byte(1);
  }
}

auto M32X::SH7604::busReadWord(u32 address) -> u32 {
  return m32x.readInternal(1, 1, address & ~1);
}

auto M32X::SH7604::busReadLong(u32 address) -> u32 {
  u32    data = m32x.readInternal(1, 1, address & ~3 | 0) << 16;
  return data | m32x.readInternal(1, 1, address & ~3 | 2) <<  0;
}

auto M32X::SH7604::busWriteByte(u32 address, u32 data) -> void {
  debugger.tracer.instruction->invalidate(address & ~1);
  if(address & 1) {
    m32x.writeInternal(0, 1, address & ~1, data << 8 | (u8)data << 0);
  } else {
    m32x.writeInternal(1, 0, address & ~1, data << 8 | (u8)data << 0);
  }
}

auto M32X::SH7604::busWriteWord(u32 address, u32 data) -> void {
  debugger.tracer.instruction->invalidate(address & ~1);
  m32x.writeInternal(1, 1, address & ~1, data);
}

auto M32X::SH7604::busWriteLong(u32 address, u32 data) -> void {
  debugger.tracer.instruction->invalidate(address & ~3 | 0);
  debugger.tracer.instruction->invalidate(address & ~3 | 2);
  m32x.writeInternal(1, 1, address & ~3 | 0, data >> 16);
  m32x.writeInternal(1, 1, address & ~3 | 2, data >>  0);
}
