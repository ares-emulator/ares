inline auto CPU::load(u32& target) const -> u32 {
  if(delay.load[0].target == &target) {
    return delay.load[0].source;
  }
  return target;
}

inline auto CPU::load(u32& target, u32 source) -> void {
  if(&target == &ipu.r[readTarget] && pipeline.instruction >> 26 < 0x20) readTarget = 0;
  if(delay.load[0].target == &target) {
    delay.load[0].target = nullptr;
  }
  delay.load[1].target = &target;
  delay.load[1].source =  source;
  delay.loadPending = 2;
}

inline auto CPU::store(u32& target, u32 source) -> void {
  if(&target == &ipu.r[readTarget]) readTarget = 0;
  if(delay.load[0].target == &target) {
    delay.load[0].target = nullptr;
  }
  target = source;
}

static constexpr auto interlockOpcode = []() {
  std::array<u8, 64> t{};
  for(u32 o : {0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x30, 0x31, 0x32, 0x33, 0x38, 0x39, 0x3a, 0x3b}) t[o] = 1;
  for(u32 o : {0x04, 0x05, 0x28, 0x29, 0x2a, 0x2b, 0x2e}) t[o] = 3;
  return t;
}();
static constexpr auto interlockSpecial = []() {
  std::array<u8, 64> t{};
  for(u32 f : {0x00, 0x02, 0x03}) t[f] = 2;
  for(u32 f : {0x08, 0x09, 0x11, 0x13}) t[f] = 1;
  for(u32 f : {0x04, 0x06, 0x07, 0x18, 0x19, 0x1a, 0x1b, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2a, 0x2b}) t[f] = 3;
  return t;
}();

auto CPU::readInterlock(u32 instruction) -> void {
  //The load delay slot reads the old register value.
  if(delay.load[0].target == &ipu.r[readTarget]) return;
  u32 rs = instruction >> 21 & 31;
  u32 rt = instruction >> 16 & 31;
  bool sourceRS = false, sourceRT = false;
  u8 flags;
  u32 opcode = instruction >> 26;
  if(opcode == 0x00) flags = interlockSpecial[instruction & 63];
  else if(opcode == 0x10 || opcode == 0x12) flags = (rs == 4 || rs == 6) ? 2 : 0;
  else flags = interlockOpcode[opcode];
  sourceRS = flags & 1;
  sourceRT = flags & 2;
  if((sourceRS && rs == readTarget) || (sourceRT && rt == readTarget)) {
    step(readLatency > 1 ? readLatency : readWritebackRemaining);
  }
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
