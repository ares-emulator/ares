inline auto CPU::load(u32& target) const -> u32 {
  if(delay.load[0].target == &target) {
    return delay.load[0].source;
  }
  return target;
}

inline auto CPU::load(u32& target, u32 source) -> void {
  if(delay.load[0].target == &target) {
    delay.load[0].target = nullptr;
  }
  delay.load[1].target = &target;
  delay.load[1].source =  source;
  delay.loadPending = 2;
}

inline auto CPU::store(u32& target, u32 source) -> void {
  if(delay.load[0].target == &target) {
    delay.load[0].target = nullptr;
  }
  target = source;
}

template<u32 N>
inline auto CPU::branch(u32 address, bool take) -> void {
  delay.branch[N].slot = true;
  delay.branch[N].take = take;
  delay.branch[N].address = address;
  if constexpr(N == 0) {
    delay.branch[1] = {};
    delay.branchPending = 1;
  } else {
    delay.branchPending = 2;
  }
}

inline auto CPU::branchCoprocessor(bool condition, s16 imm) -> void {
  bool take = (pipeline.instruction >> 16 & 1) == condition;
  branch(ipu.pd + (imm << 2), take);
}

alwaysinline auto CPU::processDelayLoad() -> void {
  if(delay.load[0].target) {
   *delay.load[0].target = delay.load[0].source;
  }
  delay.load[0] = delay.load[1];
  delay.load[1] = {};
  delay.loadPending >>= 1;
}

alwaysinline auto CPU::processDelayBranch() -> void {
  if(unlikely(delay.branch[0].take)) {
    if(unlikely(debugger.tracer.function->enabled())) debugger.function();
    if(unlikely(delay.branch[0].address == ipu.pc && ipu.pb == ipu.pc + 4)) {
      if(peek(ipu.pb) == 0 && scc.status.frame[0].interruptEnable && !exception.interruptsPending()) {
        if(!breakpoint.active && !debugger.tracer.instruction->enabled()) {
          idle();
        }
      }
    }
  }
  if(delay.branch[1].take) {
    ipu.pd = delay.branch[1].address;
    if(unlikely(!fastBooted && ipu.pd == 0x8003'0000)) {
      fastBooted = 1;
      if(disc.cd && !disc.audioCD() && system.fastBoot->value()) ipu.pd = ipu.r[31];
    }
  }
  delay.branch[0] = delay.branch[1];
  delay.branch[1] = {};
  delay.branchPending >>= 1;
}
