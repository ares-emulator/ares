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

auto CPU::readInterlock(u32 instruction) -> void {
  //The load delay slot reads the old register value.
  if(delay.load[0].target == &ipu.r[readTarget]) return;
  u32 rs = instruction >> 21 & 31;
  u32 rt = instruction >> 16 & 31;
  bool sourceRS = false, sourceRT = false;
  switch(instruction >> 26) {
  case 0x00:  //SPECIAL
    switch(instruction & 63) {
    case 0x00: case 0x02: case 0x03: sourceRT = true; break;                    //SLL, SRL, SRA
    case 0x08: case 0x09: case 0x11: case 0x13: sourceRS = true; break;         //JR, JALR, MTHI, MTLO
    case 0x04: case 0x06: case 0x07:                                            //SLLV, SRLV, SRAV
    case 0x18: case 0x19: case 0x1a: case 0x1b:                                 //MULT, MULTU, DIV, DIVU
    case 0x20: case 0x21: case 0x22: case 0x23:                                 //ADD, ADDU, SUB, SUBU
    case 0x24: case 0x25: case 0x26: case 0x27: case 0x2a: case 0x2b:           //AND, OR, XOR, NOR, SLT, SLTU
      sourceRS = sourceRT = true;
    }
    break;
  case 0x01: case 0x06: case 0x07:                                              //REGIMM, BLEZ, BGTZ
  case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e:  //ADDI/U, SLTI/U, ANDI, ORI, XORI
  case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:  //LB, LH, LWL, LW, LBU, LHU, LWR
  case 0x30: case 0x31: case 0x32: case 0x33:                                   //LWC0..3
  case 0x38: case 0x39: case 0x3a: case 0x3b:                                   //SWC0..3
    sourceRS = true; break;
  case 0x04: case 0x05:                                                         //BEQ, BNE
  case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2e:                        //SB, SH, SWL, SW, SWR
    sourceRS = sourceRT = true; break;
  case 0x10: case 0x12:                                                         //COP0, COP2
    sourceRT = rs == 4 || rs == 6; break;                                       //MTC, CTC
  }
  if((sourceRS && rs == readTarget) || (sourceRT && rt == readTarget)) step(readLatency);
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
