//Adapted from ares-64 1c9b9637476a83f7b80d3db1115a0f71529820d9.
//CPU debug engine (see debug-control.hpp for the execution model).

auto CPU::DebugControl::power() -> void {
  //definitions survive power/state loads; prior hits and stops do not
  halted = false;
  stopValid = false;
  stop = {};
  pendingAccess = {};
  narrowLength = 0;
  stepping = false;
  stepRemaining = 0;
  haltRequested = false;
  suppressBreakOnce = false;
  sliceCount = 0;
  clearHits();
  updateActive();
}

auto CPU::DebugControl::clearHits() -> void {
  for(auto& b : breakpoints) b.hits = 0;
  for(auto& w : watchpoints) w.hits = 0;
}

auto CPU::DebugControl::updateActive() -> void {
  watchActive = !watchpoints.empty();
  active = !breakpoints.empty() || watchActive || stepping || haltRequested;
  gate = active || halted || sliceBudget != 0;
}

//Accepts low RDRAM physical addresses, KSEG0/KSEG1 (32-bit) and their sign-extended
//64-bit spellings. TLB-mapped, MMIO and wrapped ranges are rejected.
auto CPU::DebugControl::normalizeAddress(u64 address, u32 length, u32& paddr, string& error) const -> bool {
  u64 ramSize = rdram.ram.size;
  u64 pa;
  if(address < 0x8000'0000ull) {
    pa = address;
  } else if(address < 0xc000'0000ull) {
    pa = address & 0x1fff'ffff;
  } else if(address >= 0xffff'ffff'8000'0000ull && address < 0xffff'ffff'c000'0000ull) {
    pa = address & 0x1fff'ffff;
  } else {
    error = {"address 0x", hex(address), " is not RDRAM: use a physical address, KSEG0 (0x80000000-0x9fffffff) or KSEG1 (0xa0000000-0xbfffffff); TLB-mapped and MMIO addresses are unsupported"};
    return false;
  }
  if(length == 0) {
    error = "length must be at least 1";
    return false;
  }
  if(pa >= ramSize || pa + length > ramSize) {
    error = {"range 0x", hex(pa), "+", length, " exceeds installed RDRAM (0x", hex(ramSize), " bytes)"};
    return false;
  }
  paddr = (u32)pa;
  return true;
}

auto CPU::DebugControl::addBreakpoint(u64 address, Condition condition, string& error) -> u32 {
  if(breakpoints.size() >= MaxBreakpoints) { error = {"breakpoint table full (", MaxBreakpoints, ")"}; return 0; }
  if(address & 3) { error = "breakpoint address must be 4-byte aligned"; return 0; }
  u32 paddr;
  if(!normalizeAddress(address, 4, paddr, error)) return 0;
  for(auto& p : condition.all) {
    if(p.field == Field::Value) { error = "breakpoint conditions cannot use the access value field"; return 0; }
  }
  Breakpoint b;
  b.id = nextBreakpointId++;
  b.address = address;
  b.paddr = paddr;
  b.condition = std::move(condition);
  breakpoints.push_back(std::move(b));
  updateActive();
  return breakpoints.back().id;
}

auto CPU::DebugControl::removeBreakpoint(u32 id) -> bool {
  for(auto it = breakpoints.begin(); it != breakpoints.end(); ++it) {
    if(it->id != id) continue;
    breakpoints.erase(it);
    updateActive();
    return true;
  }
  return false;
}

auto CPU::DebugControl::addWatchpoint(u64 address, u32 length, u32 kind, Condition condition, string& error) -> u32 {
  if(watchpoints.size() >= MaxWatchpoints) { error = {"watchpoint table full (", MaxWatchpoints, ")"}; return 0; }
  if(!kind || (kind & ~(AccessRead | AccessWrite))) { error = "watch kind must be read, write or access"; return 0; }
  if(length > 0x10000) { error = "watch length is limited to 65536 bytes"; return 0; }
  u32 paddr;
  if(!normalizeAddress(address, length, paddr, error)) return 0;
  Watchpoint w;
  w.id = nextWatchpointId++;
  w.address = address;
  w.paddr = paddr;
  w.length = length;
  w.kind = kind;
  w.condition = std::move(condition);
  watchpoints.push_back(std::move(w));
  updateActive();
  return watchpoints.back().id;
}

auto CPU::DebugControl::removeWatchpoint(u32 id) -> bool {
  for(auto it = watchpoints.begin(); it != watchpoints.end(); ++it) {
    if(it->id != id) continue;
    watchpoints.erase(it);
    updateActive();
    return true;
  }
  return false;
}

auto CPU::DebugControl::requestStepInstructions(u64 count) -> void {
  stepping = true;
  stepRemaining = count;
  updateActive();
}

auto CPU::DebugControl::requestHalt() -> void {
  haltRequested = true;
  updateActive();
}

auto CPU::DebugControl::resume() -> void {
  if(halted && stopValid && stop.reason == StopReason::Breakpoint) {
    //step off the breakpoint we are sitting on, exactly once
    suppressBreakOnce = true;
    suppressPc = self.ipu.pc;
  }
  halted = false;
  haltRequested = false;
  stepping = false;
  stepRemaining = 0;
  updateActive();
}

auto CPU::DebugControl::exitMain() -> bool {
  if(halted) return true;
  if(sliceBudget && ++sliceCount >= sliceBudget) {
    sliceCount = 0;
    return true;
  }
  return false;
}

auto CPU::DebugControl::controlRegister(u32 index, u64& value) const -> bool {
  auto& scc = self.scc;
  if(index == 1) return false;  //Random: reading it advances the emulator RNG
  if(index == 12) {
    n64 data;
    data.bit( 0)    = scc.status.interruptEnable;
    data.bit( 1)    = scc.status.exceptionLevel;
    data.bit( 2)    = scc.status.errorLevel;
    data.bit( 3, 4) = scc.status.privilegeMode;
    data.bit( 5)    = scc.status.userExtendedAddressing;
    data.bit( 6)    = scc.status.supervisorExtendedAddressing;
    data.bit( 7)    = scc.status.kernelExtendedAddressing;
    data.bit( 8,15) = scc.status.interruptMask;
    data.bit(16)    = scc.status.de;
    data.bit(17)    = scc.status.ce;
    data.bit(18)    = scc.status.condition;
    data.bit(20)    = scc.status.softReset;
    data.bit(21)    = scc.status.tlbShutdown;
    data.bit(22)    = scc.status.vectorLocation;
    data.bit(24)    = scc.status.instructionTracing;
    data.bit(25)    = scc.status.reverseEndian;
    data.bit(26)    = scc.status.floatingPointMode;
    data.bit(27)    = scc.status.lowPowerMode;
    data.bit(28)    = scc.status.enable.coprocessor0;
    data.bit(29)    = scc.status.enable.coprocessor1;
    data.bit(30)    = scc.status.enable.coprocessor2;
    data.bit(31)    = scc.status.enable.coprocessor3;
    value = data;
    return true;
  }
  //every other index of getControlRegister() is a pure read of the SCC state
  value = const_cast<CPU&>(self).getControlRegister(index);
  return true;
}

auto CPU::DebugControl::snapshot() const -> Snapshot {
  Snapshot s;
  s.valid = true;
  for(u32 n : range(32)) s.gpr[n] = self.ipu.r[n].u64;
  s.gpr[0] = 0;
  s.lo = self.ipu.lo.u64;
  s.hi = self.ipu.hi.u64;
  s.pc = self.ipu.pc;
  s.nextPc = self.pipeline.nextpc;
  s.delaySlot = self.pipeline.state & Pipeline::DelaySlot;
  for(u32 n : range(32)) {
    if(controlRegister(n, s.cop0[n])) s.cop0Valid |= 1u << n;
  }
  for(u32 n : range(32)) s.fpr[n] = self.fpu.r[n].u64;
  s.fcr0 = const_cast<CPU&>(self).getControlRegisterFPU(0);
  s.fcr31 = const_cast<CPU&>(self).getControlRegisterFPU(31);
  return s;
}

auto CPU::DebugControl::takeStop(StopReason reason) -> void {
  stop.reason = reason;
  stop.snapshot = snapshot();
  stop.frame = executionCoordinate.frame;
  stop.instructions = instructionsRetired;
  stopValid = true;
  halted = true;
  updateActive();
}

auto CPU::DebugControl::evaluate(const Condition& condition, const AccessHit* access) const -> bool {
  for(auto& p : condition.all) {
    u64 v = 0;
    switch(p.field) {
    case Field::Pc:    v = currentPc; break;
    case Field::Gpr:   v = p.index ? self.ipu.r[p.index & 31].u64 : 0; break;
    case Field::Hi:    v = self.ipu.hi.u64; break;
    case Field::Lo:    v = self.ipu.lo.u64; break;
    case Field::Value: if(!access) return false; v = access->value; break;
    }
    v &= p.mask;
    u64 c = p.value & p.mask;
    bool ok = false;
    switch(p.op) {
    case Op::Eq: ok = v == c; break;
    case Op::Ne: ok = v != c; break;
    case Op::Lt: ok = v <  c; break;
    case Op::Le: ok = v <= c; break;
    case Op::Gt: ok = v >  c; break;
    case Op::Ge: ok = v >= c; break;
    }
    if(!ok) return false;
  }
  return true;
}

//Called with the translated instruction-fetch address, before fetch/dispatch, so a
//stop here leaves the pipeline untouched.
auto CPU::DebugControl::beforeInstruction(PhysAccess access) -> bool {
  currentPc = self.ipu.pc;
  narrowLength = 0;

  if(haltRequested) {
    haltRequested = false;
    stop = {};
    takeStop(StopReason::Requested);
    return true;
  }
  if(stepping && stepRemaining == 0) {
    stop = {};
    takeStop(StopReason::InstructionsDone);
    return true;
  }

  bool suppressed = suppressBreakOnce && suppressPc == currentPc;
  suppressBreakOnce = false;
  if(!breakpoints.empty() && !suppressed) {
    u32 hitId = 0;
    std::vector<u32> others;
    for(auto& b : breakpoints) {
      if(b.paddr != access.paddr) continue;
      if(!evaluate(b.condition, nullptr)) continue;
      b.hits++;
      if(!hitId) hitId = b.id;
      else others.push_back(b.id);
    }
    if(hitId) {
      stop = {};
      stop.breakpointId = hitId;
      stop.otherBreakpointIds = std::move(others);
      stop.breakpointPc = currentPc;
      takeStop(StopReason::Breakpoint);
      return true;
    }
  }
  return false;
}

auto CPU::DebugControl::afterInstruction(u64 pc, u32 instruction) -> void {
  instructionsRetired++;
  narrowLength = 0;
  if(stepping && stepRemaining) stepRemaining--;

  if(pendingAccess.valid) {
    stop = {};
    stop.access = std::move(pendingAccess);
    pendingAccess = {};
    takeStop(StopReason::Watchpoint);
    return;
  }
  if(stepping && !stepRemaining) {
    stop = {};
    takeStop(StopReason::InstructionsDone);
  }
}

template<u32 Size>
static inline auto debugSizeMask() -> u64 {
  if constexpr(Size >= 8) return ~0ull;
  else return (1ull << (Size * 8)) - 1;
}

template<u32 Size>
auto CPU::DebugControl::onRead(const PhysAccess& access, u64 value) -> void {
  u64 vaddr = access.vaddr;
  u32 paddr = access.paddr;
  u32 size = Size;
  value &= debugSizeMask<Size>();
  //LWL/LWR/LDL/LDR read the aligned container but only consume part of it
  if(narrowLength && narrowVaddr >= access.vaddr && narrowVaddr - access.vaddr < Size
  && narrowVaddr - access.vaddr + narrowLength <= Size) {
    u32 offset = narrowVaddr - access.vaddr;
    vaddr += offset;
    paddr += offset;
    size = narrowLength;
    u32 shift = (Size - offset - narrowLength) * 8;
    value = (value >> shift) & (size >= 8 ? ~0ull : (1ull << (size * 8)) - 1);
  }
  for(auto& w : watchpoints) {
    if(!(w.kind & AccessRead)) continue;
    if(paddr + size <= w.paddr || paddr >= w.paddr + w.length) continue;
    AccessHit hit;
    hit.kind = AccessRead;
    hit.vaddr = vaddr;
    hit.paddr = paddr;
    hit.size = size;
    hit.value = value;
    if(!evaluate(w.condition, &hit)) continue;
    w.hits++;
    if(!pendingAccess.valid) {
      hit.valid = true;
      hit.watchId = w.id;
      hit.pc = currentPc;
      hit.instruction = currentInstruction;
      pendingAccess = std::move(hit);
    } else if(pendingAccess.watchId != w.id) {
      auto& others = pendingAccess.otherIds;
      if(std::find(others.begin(), others.end(), w.id) == others.end()) others.push_back(w.id);
    }
  }
}

template<u32 Size>
auto CPU::DebugControl::onWrite(const PhysAccess& access, u64 data) -> void {
  data &= debugSizeMask<Size>();
  u32 paddr = access.paddr;
  bool haveOld = false;
  u64 old = 0;
  for(auto& w : watchpoints) {
    if(!(w.kind & AccessWrite)) continue;
    if(paddr + Size <= w.paddr || paddr >= w.paddr + w.length) continue;
    if(!haveOld && paddr + Size <= rdram.ram.size) {
      //side-effect-free CPU view of the bytes about to be replaced
      Thread dummyThread{};
      if(access.cache) old = self.dcache.readDebug<Size>(access.vaddr, paddr);
      else old = bus.read<Size>(paddr, dummyThread, RBusDevice::ARES_DEBUGGER);
      old &= debugSizeMask<Size>();
      haveOld = true;
    }
    AccessHit hit;
    hit.kind = AccessWrite;
    hit.vaddr = access.vaddr;
    hit.paddr = paddr;
    hit.size = Size;
    hit.value = data;
    hit.oldValue = old;
    hit.hasOld = haveOld;
    if(!evaluate(w.condition, &hit)) continue;
    w.hits++;
    if(!pendingAccess.valid) {
      hit.valid = true;
      hit.watchId = w.id;
      hit.pc = currentPc;
      hit.instruction = currentInstruction;
      pendingAccess = std::move(hit);
    } else if(pendingAccess.watchId != w.id) {
      auto& others = pendingAccess.otherIds;
      if(std::find(others.begin(), others.end(), w.id) == others.end()) others.push_back(w.id);
    }
  }
}

