#include <nall/gdb/server.hpp>

auto M32X::SH7604::load(Node::Object parent, string name, string bootFile) -> void {
  node = parent->append<Node::Object>(name);
  if(auto fp = system.pak->read(bootFile)) {
    bootROM.allocate(fp->size() >> 1);
    for(auto address : range(bootROM.size())) bootROM.program(address, fp->readm(2L));
  }
  debugger.load(node);
  
  if (m32x.shm.active()) {
    initDebugHooks();
  }
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
  
  if (m32x.shm.active()) {
    if (m32x.vdp.vblank) {
      nall::GDB::server.updateLoop();
    }
    
    nall::GDB::server.reportPC(regs.PC);
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
  Thread::create((system.frequency() / 7.0) * 3.0, {&M32X::SH7604::main, this});

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
  Thread::restart({&M32X::SH7604::main, this});
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
  if (m32x.shm.active()) {
    nall::GDB::server.reportMemRead(address, 1);
  }
  if(address & 1) {
    return m32x.readInternal(0, 1, address & ~1).byte(0);
  } else {
    return m32x.readInternal(1, 0, address & ~1).byte(1);
  }
}

auto M32X::SH7604::busReadWord(u32 address) -> u32 {
  if (m32x.shm.active()) {
    nall::GDB::server.reportMemRead(address, 2);
  }
  return m32x.readInternal(1, 1, address & ~1);
}

auto M32X::SH7604::busReadLong(u32 address) -> u32 {
  if (m32x.shm.active()) {
    nall::GDB::server.reportMemRead(address, 4);
  }
  u32    data = m32x.readInternal(1, 1, address & ~3 | 0) << 16;
  return data | m32x.readInternal(1, 1, address & ~3 | 2) <<  0;
}

auto M32X::SH7604::busWriteByte(u32 address, u32 data) -> void {
  if (m32x.shm.active()) {
    nall::GDB::server.reportMemWrite(address, 1);
  }
  debugger.tracer.instruction->invalidate(address & ~1);
  if(address & 1) {
    m32x.writeInternal(0, 1, address & ~1, data << 8 | (u8)data << 0);
  } else {
    m32x.writeInternal(1, 0, address & ~1, data << 8 | (u8)data << 0);
  }
}

auto M32X::SH7604::busWriteWord(u32 address, u32 data) -> void {
  if (m32x.shm.active()) {
    nall::GDB::server.reportMemWrite(address, 2);
  }
  debugger.tracer.instruction->invalidate(address & ~1);
  m32x.writeInternal(1, 1, address & ~1, data);
}

auto M32X::SH7604::busWriteLong(u32 address, u32 data) -> void {
  if (m32x.shm.active()) {
    nall::GDB::server.reportMemWrite(address, 4);
  }
  debugger.tracer.instruction->invalidate(address & ~3 | 0);
  debugger.tracer.instruction->invalidate(address & ~3 | 2);
  m32x.writeInternal(1, 1, address & ~3 | 0, data >> 16);
  m32x.writeInternal(1, 1, address & ~3 | 2, data >>  0);
}

auto M32X::SH7604::initDebugHooks() -> void {

  // See: https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format
  nall::GDB::server.hooks.targetXML = []() -> string {
    return "<target version=\"1.0\">"
      "<architecture>sheb</architecture>"
    "</target>";
  };
  
  nall::GDB::server.hooks.read = [](u64 address, u32 byteCount) -> string {
    address = (s32)address;

    string res{};
    res.resize(byteCount * 2);
    char* resPtr = res.begin();

    for(u32 i : range(byteCount)) {
      auto val = m32x.debugRead(address++);
      hexByte(resPtr, val);
      resPtr += 2;
    }

    return res;
  };
  
  nall::GDB::server.hooks.regRead = [this](u32 regIdx) {
    if(regIdx < 16) {
      return hex(regs.R[regIdx], 16, '0');
    }

    switch (regIdx)
    {
      case 16: { // PC
        auto pcOverride = nall::GDB::server.getPcOverride();
        return hex(pcOverride ? pcOverride.get() : regs.PC, 16, '0');
      }
      case 17: return hex(regs.PR, 16, '0');
      case 18: return hex(regs.GBR, 16, '0');
      case 19: return hex(regs.VBR, 16, '0');
      case 20: return hex(regs.MACL, 16, '0');
      case 21: return hex(regs.MACH, 16, '0');
      case 22: return hex(regs.CCR, 16, '0');
      case 23: return hex((u32)regs.SR, 16, '0');
      case 24: return hex(regs.PPC, 16, '0');
      case 25: return hex(regs.PPM, 16, '0');
      case 26: return hex(regs.ET, 16, '0');
      case 27: return hex(regs.ID, 16, '0');
    }

    return string{"0000000000000000"};
  };
  
  nall::GDB::server.hooks.regReadGeneral = []() {
    string res{};
    for(auto i : range(28)) {
      res.append(nall::GDB::server.hooks.regRead(i));
    }
    return res;
  };
}
