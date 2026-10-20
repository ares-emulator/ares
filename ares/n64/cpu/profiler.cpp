// Bounded CPU cost accounting adapted from ares-64 1c9b9637476a83f7b80d3db1115a0f71529820d9.
// Emulation-thread only. Symbol loading, wait classification and UI event rings are deferred.

auto CPU::Profiler::now() -> u64 {
  // Master-clock timebase (187.5 MHz). synchronize() folds cpu.clock into
  // profile.cpuCycles as (clocks >> 1), i.e. CPU cycles, so scale that back up
  // by 2 and add the still-pending master clocks. A COUNT read (flushCount)
  // folds the pending clocks early without resetting cpu.clock, tracking how
  // much it took in countClock: subtract that or those clocks would be counted
  // twice until the next synchronize, which made timestamps jump ahead and then
  // fall back. This may still step backwards by at most 1 master clock at a
  // sync boundary (the truncated low bit); callers guard their subtractions.
  return (u64)(cpu.profile.cpuCycles * 2 + cpu.clock - cpu.countClock);
}

auto CPU::Profiler::onInstruction(u64 address, u32 instruction) -> void {
  u32 pc32 = (u32)address;

  // Per-thread call-stack tracking. The live $sp identifies which stack we are on.
  // Suppressed while an exception handler is open: its code runs on the kernel/interrupt stack
  // but is meant to nest under the interrupted function, so we keep it on the active
  // stack and let the eret below hand off to the resumed thread on the next step.
  u32 sp = (u32)cpu.ipu.r[29].u64;  //$sp
  if(excActive == 0) {
    if(!haveSp) { haveSp = true; spLo = spHi = sp; }
    else if(u64(sp) + regionSlack < spLo || u64(sp) > u64(spHi) + regionSlack) switchStack(sp);
    else { if(sp < spLo) spLo = sp; if(sp > spHi) spHi = sp; }
  }

  // Stack-pointer backstop for returns the retAddr match can never see (missed `jr` hooks, longjmp, a thread blocking mid-call):
  // once the live $sp has risen above a frame's recorded caller-$sp, that call and everything nested in it have unwound, so pop them.
  // Stops at exception frames (carry no $sp, popped by onEret), keeping the interrupted thread's frames beneath a handler intact
  while(!callStack.empty() && !callStack.back().isException && sp > callStack.back().sp) {
    popFrame();
  }

  // Return detection by *arrival* at a call's return address, rather than by
  // catching the `jr $ra` instruction. The JIT does not reliably emit the
  // per-instruction hook for a `jr` reached as the fall-through of a preceding
  // branch (back-to-back branches), so the return itself can be invisible.
  // but the return *target* is always a normal instruction in the (instrumented)
  // caller. When execution arrives at the top frame's recorded return address,
  // that call has returned: pop it. One pop per arrival so recursion that returns
  // to a shared call site unwinds a single level at a time. Exception frames are
  // popped by onEret, not here, and never matched (their retAddr is 0).
  if(!callStack.empty() && !callStack.back().isException && callStack.back().retAddr == pc32) {
    popFrame();
  }

  u32 op = instruction >> 26;
  bool isJAL  = (op == 0x03);
  bool isJALR = (op == 0x00) && ((instruction & 0x3f) == 0x09);

  if(isJAL || isJALR) {
    // Only standard calls that link into $ra (rd=31 for JALR) form a call/return
    // pair we can balance; a JALR linking elsewhere returns via a register we
    // don't track, so treating it as a call would just leak a frame.
    if(isJALR && ((instruction >> 11) & 31) != 31) return;
    if(callStack.size() >= maxStackDepth) { droppedStackFrames++; return; }  //runaway guard
    u32 target = isJAL
      ? (((pc32 + 4) & 0xf000'0000) | ((instruction & 0x03ff'ffff) << 2))
      : (u32)cpu.ipu.r[(instruction >> 21) & 31].u64;  //JALR rs holds the target
    Frame f;
    f.funcAddr = target;
    f.retAddr = pc32 + 8;  //where the callee returns to (after the delay slot)
    f.sp = sp;             //caller's $sp at the call site (see the unwind backstop above)
    f.entryCycle = f.segmentCycle = now();
    callStack.push_back(f);
  }
}

auto CPU::Profiler::onBusAccess(bool toRDRAM, u64 bytes) -> void {
  if(callStack.empty()) return;  //bytes outside any tracked frame are dropped
  if(toRDRAM) callStack.back().bytesOutOwn += bytes;
  else        callStack.back().bytesInOwn  += bytes;
}

auto CPU::Profiler::commitFrame(Frame& frame, Frame* parent, u64 time) -> void {
  u64 incl = time > frame.segmentCycle ? time - frame.segmentCycle : 0;
  u64 excl = incl > frame.childCycles ? incl - frame.childCycles : 0;
  bool spin = false;  // Symbol-based wait classification is not available in this service slice.
  u64 subtreeWait = frame.childWait;
  // Bytes: own = exclusive (this function's code), incl = own + returned callees.
  u64 inclBytesIn  = frame.bytesInOwn  + frame.childBytesIn;
  u64 inclBytesOut = frame.bytesOutOwn + frame.childBytesOut;
  u64 inclCache[CacheKinds];
  for(u32 k : range(CacheKinds)) inclCache[k] = frame.cacheOwn[k] + frame.childCache[k];
  auto add = [&](std::unordered_map<u32, FuncStat>& m) {
    if(!m.count(frame.funcAddr) && m.size() >= maxFunctions) { droppedFunctionUpdates++; return; }
    auto& st = m[frame.funcAddr];
    st.addr = frame.funcAddr;
    st.isSpin = spin;
    st.callCount += frame.newCall;
    st.inclCycles += incl;
    st.exclCycles += excl;
    st.waitCycles += subtreeWait;
    st.inclBytesIn  += inclBytesIn;
    st.inclBytesOut += inclBytesOut;
    st.exclBytesIn  += frame.bytesInOwn;
    st.exclBytesOut += frame.bytesOutOwn;
    for(u32 k : range(CacheKinds)) {
      st.inclCacheBytes[k] += inclCache[k];
      st.exclCacheBytes[k] += frame.cacheOwn[k];
    }
  };
  bool hasData = frame.newCall || incl || inclBytesIn || inclBytesOut;
  for(u32 k : range(CacheKinds)) hasData |= inclCache[k] != 0;
  if(hasData) {
    if(frameCount < maxFrames) add(stats);
    add(frameAccum);
  }
  if(parent) {
    parent->childCycles += incl;
    parent->childWait += subtreeWait;
    parent->childBytesIn += inclBytesIn;
    parent->childBytesOut += inclBytesOut;
    for(u32 k : range(CacheKinds)) parent->childCache[k] += inclCache[k];
  }
  frame.segmentCycle = time;
  frame.newCall = false;
  frame.childCycles = frame.childWait = 0;
  frame.bytesInOwn = frame.bytesOutOwn = frame.childBytesIn = frame.childBytesOut = 0;
  for(u32 k : range(CacheKinds)) frame.cacheOwn[k] = frame.childCache[k] = 0;
}

auto CPU::Profiler::flushFrames() -> void {
  u64 time = now();
  for(size_t i = callStack.size(); i > 0; i--) {
    commitFrame(callStack[i - 1], i > 1 ? &callStack[i - 2] : nullptr, time);
  }
}

auto CPU::Profiler::popFrame() -> bool {
  commitFrame(callStack.back(), callStack.size() > 1 ? &callStack[callStack.size()-2] : nullptr, now());
  bool exception = callStack.back().isException;
  callStack.pop_back();
  if(exception && excActive) excActive--;
  return exception;
}

auto CPU::Profiler::switchStack(u32 sp) -> void {
  flushFrames();
  suspended.push_back({spLo, spHi, ++stackClock, std::move(callStack)});
  callStack.clear();  //callStack was moved-from; make it valid+empty

  s32 found = -1;
  for(u32 i = 0; i + 1 < suspended.size(); i++) {  //skip the entry just parked (last)
    auto& s = suspended[i];
    if(u64(sp) + regionSlack >= s.lo && u64(sp) <= u64(s.hi) + regionSlack) { found = (s32)i; break; }
  }
  if(found >= 0) {
    auto& s = suspended[(u32)found];
    callStack = std::move(s.frames);
    // Suspended threads consume no CPU cycles.
    for(auto& f : callStack) f.segmentCycle = now();
    spLo = s.lo; spHi = s.hi;
    suspended.erase(suspended.begin() + found);
  } else {
    spLo = spHi = sp;  //unseen thread: observe its calls fresh
  }

  // Bound memory: drop the least-recently-active parked thread's frames.
  while(suspended.size() > maxStacks) {
    u32 oldest = 0;
    for(u32 i = 1; i < suspended.size(); i++) if(suspended[i].used < suspended[oldest].used) oldest = i;
    droppedStackFrames += suspended[oldest].frames.size();
    suspended.erase(suspended.begin() + oldest);
  }
}

auto CPU::Profiler::exceptionEntryAddr(u32 code) -> u32 {
  if(code != 0) return kExcBase | (0x40 + (code & 0x3f));  //synchronous exception
  // Interrupt: identify the firing CPU interrupt source (and RCP sub-source).
  u32 pend = (u32)(cpu.scc.cause.interruptPending & cpu.scc.status.interruptMask);
  for(u32 bit = 0; bit < 8; bit++) {
    if(!(pend & (1u << bit))) continue;
    if(bit == CPU::Interrupt::RCP) {
      u32 irqs = mi.activeIRQs();
      for(u32 i = 0; i < 6; i++) if(irqs & (1u << i)) return kExcBase | (0x10 + i);
      return kExcBase | 0x20;  //RCP but sub-source already cleared
    }
    return kExcBase | (0x20 + bit);  //non-RCP CPU interrupt (timer / sw / cart)
  }
  return kExcBase | 0x30;  //no specific source found
}

auto CPU::Profiler::labelFor(u32 addr) -> string {
  if(!isExceptionAddr(addr)) {
    return string{"0x", hex(addr, 8L)};
  }
  u32 id = addr & 0xffff;
  if(id >= 0x10 && id < 0x16) {
    static const char* irqNames[6] = {"SP", "SI", "AI", "VI", "PI", "DP"};
    return string{"[IRQ: ", irqNames[id - 0x10], "]"};
  }
  if(id >= 0x20 && id < 0x28) {
    static const char* cpuIrq[8] = {"SW0", "SW1", "RCP", "cart", "reset", "RDB", "RDB", "timer"};
    return string{"[IRQ: ", cpuIrq[id - 0x20], "]"};
  }
  if(id >= 0x40 && id < 0x80) return string{"[exception ", id - 0x40, "]"};
  return string{"[interrupt]"};
}

auto CPU::Profiler::onException(u32 code) -> void {
  // Push a synthetic frame for the handler so its time nests under (and is thus
  // subtracted from) the interrupted function, and shows as its own entry.
  if(callStack.size() >= maxStackDepth) { droppedStackFrames++; return; }
  Frame f;
  f.funcAddr = exceptionEntryAddr(code);
  f.entryCycle = f.segmentCycle = now();
  f.isException = true;
  callStack.push_back(f);
  excActive++;  //freeze stack-switching until the matching eret (see onInstruction)
}

auto CPU::Profiler::onEret() -> void {
  // Unwind back to and including the most recent exception frame. (Normally the
  // handler has balanced its own calls, so that frame is already on top.)
  bool hasExc = false;
  for(auto& f : callStack) if(f.isException) { hasExc = true; break; }
  if(!hasExc) return;
  while(!callStack.empty()) {
    if(popFrame()) break;  //stop after the exception frame is popped
  }
}

auto CPU::Profiler::onFrame() -> void {
  if(!enabled) return;
  flushFrames(); frameStats.swap(frameAccum); frameAccum.clear();
  if(frameCount < maxFrames) frameCount++;
}
auto CPU::Profiler::setEnabled(bool value) -> void {
  if(value == enabled) return;
  if(!value) flushFrames();
  enabled = value;
  callStack.clear(); suspended.clear(); haveSp = false; excActive = 0;
  spLo = spHi = 0; stackClock = 0;
  cpu.updatePrologueHook();
}
auto CPU::Profiler::power() -> void {
  callStack.clear(); suspended.clear(); haveSp = false; excActive = 0;
  spLo = spHi = 0; stackClock = 0; clearStats();
}
auto CPU::Profiler::clearStats() -> void {
  stats.clear(); frameStats.clear(); frameAccum.clear();
  auto reset = [&](auto& frames) {
    for(auto& f : frames) {
      Frame clean; clean.funcAddr = f.funcAddr; clean.retAddr = f.retAddr;
      clean.sp = f.sp; clean.entryCycle = clean.segmentCycle = now();
      clean.isException = f.isException; clean.newCall = false; f = clean;
    }
  };
  reset(callStack); for(auto& s : suspended) reset(s.frames);
  frameCount = droppedFunctionUpdates = droppedStackFrames = 0;
}
