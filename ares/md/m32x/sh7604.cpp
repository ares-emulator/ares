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
  if(!m32x.io.adapterReset) return step(1000);

  if(!regs.ID) {
    #define raise(type, level, vector, ...) \
      if(SH2::inDelaySlot()) SH2::cyclesUntilRecompilerExit = 0; \
      else { \
        debugger.interrupt(type); \
        __VA_ARGS__; \
        return regs.ET = 1, interrupt(level, vector); \
      }
    if(irq.vres.active && irq.vres.enable) {
      raise("VRES", 14, 71, irq.vres.active = 0);
    } else if(irq.vint.active && irq.vint.enable && regs.SR.I < 12) {
      raise("VINT", 12, 70);
    } else if(irq.hint.active && irq.hint.enable && regs.SR.I < 10) {
      raise("HINT", 10, 69);
    } else if(irq.cmd.active && irq.cmd.enable && regs.SR.I < 8) {
      raise("CMD", 8, 68);
    } else if(irq.pwm.active && irq.pwm.enable && regs.SR.I < 6) {
      raise("PWM", 6, 67);
    }
    #undef raise
  }

  SH2::instruction();
  SH2::intc.run();
  SH2::dmac.run();
  if(m32x.shm.active()) m32x.shm.dmac.dreq[1] = 0;
  if(m32x.shs.active()) m32x.shs.dmac.dreq[1] = 0;
}

auto M32X::SH7604::instructionPrologue(u16 instruction) -> void {
  debugger.instruction(instruction);
}

auto M32X::SH7604::internalStep(u32 clocks) -> void {
  if(SH2::Accuracy::Recompiler && m32x.shm.recompiler.enabled) {
    regs.CCR += clocks;
    return;
  }

  step(clocks);
}

auto M32X::SH7604::step(u32 clocks) -> void {
  SH2::frt.run(clocks);
  SH2::wdt.run(clocks);
  Thread::step(clocks);

  cyclesUntilSh2Sync -= clocks;
  cyclesUntilM68kSync -= clocks;

  m32x.vdp.framebufferWait -= min(clocks, m32x.vdp.framebufferWait);

  if(cyclesUntilSh2Sync <= 0) {
    cyclesUntilSh2Sync = minCyclesBetweenSh2Syncs;
    if (m32x.shm.active()) Thread::synchronize(m32x.shs);
    if (m32x.shs.active()) Thread::synchronize(m32x.shm);
  }

  if(cyclesUntilM68kSync <= 0) {
    cyclesUntilM68kSync = minCyclesBetweenM68kSyncs;
    Thread::synchronize(cpu);
  }
}

auto M32X::SH7604::power(bool reset) -> void {
  Thread::create((system.frequency() / 7.0) * 3.0, std::bind_front(&M32X::SH7604::main, this));

  //When tweaking these values, make sure to test the following problematic games:
  // Brutal  - Check for hang in attract mode
  // Chaotix - Check for hang on intro screen or level loading transitions

  SH2::recompilerStepCycles =  200; //Recompiler will force an exit after at least N cycles have passed
  minCyclesBetweenSh2Syncs  =   10; //Do not sync SH2s more than once every N cycles
  minCyclesBetweenM68kSyncs =   50; //Do not sync M68K more than once every N cycles
  SH2::power(reset);
  irq = {};
  irq.vres.enable = 1;
}

auto M32X::SH7604::restart() -> void {
  SH2::power(true);
  irq = {};
  irq.vres.enable = 1;
  Thread::restart(std::bind_front(&M32X::SH7604::main, this));
}

auto M32X::SH7604::syncAll(bool force) -> void {
  SH2::cyclesUntilRecompilerExit = 0;

  if(SH2::Accuracy::Recompiler && m32x.shm.recompiler.enabled && force) {
    cyclesUntilSh2Sync = 0;
    cyclesUntilM68kSync = 0;
    step(regs.CCR);
    regs.CCR = 0;
  }
}

auto M32X::SH7604::syncOtherSh2(bool force) -> void {
  SH2::cyclesUntilRecompilerExit = 0;

  if(SH2::Accuracy::Recompiler && m32x.shm.recompiler.enabled && force) {
    cyclesUntilSh2Sync = 0;
    step(regs.CCR);
    regs.CCR = 0;
  }
}

auto M32X::SH7604::syncM68k(bool force) -> void {
  SH2::cyclesUntilRecompilerExit = 0;

  if(SH2::Accuracy::Recompiler && m32x.shm.recompiler.enabled && force) {
    cyclesUntilM68kSync = 0;
    step(regs.CCR);
    regs.CCR = 0;
  }
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
