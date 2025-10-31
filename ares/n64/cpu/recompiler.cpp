
auto CPU::Recompiler::pool(u32 address) -> Pool* {
  auto& pool = pools[address >> 8 & 0x1fffff];
  if(!pool) {
    pool = (Pool*)allocator.acquire(sizeof(Pool));
    memory::jitprotect(false);
    *pool = {};
    memory::jitprotect(true);
  }
  return pool;
}

auto CPU::Recompiler::blockCacheKey(u32 address, u32 state) -> u64 {
  return u64(address) | (u64(state) << 32);
}

auto CPU::Recompiler::block(u64 vaddr, u32 address) -> Block* {
  u32 state = jitContext.stateBits;
  u32 idx = (address >> 2) & 0x3f;
  assert(idx < sizeof(Pool::rows)/sizeof(Pool::rows[0]));

  {
    Pool* p = pool(address);
    if (auto row = p->rows[idx]) {
      if (row->stateBits == state) {
        assert(row->block);
        return row->block;
      } else {
        auto blockIt = blockCache.find(blockCacheKey(address, state));
        if (blockIt != blockCache.end()) {
          row->block = blockIt->second;
          row->stateBits = state;
          return row->block;
        }
      }
    }
  }

  auto block = emit(vaddr, address);
  if (block) {
    Pool* p = pool(address);
    auto row = (PoolRow*)allocator.acquire(sizeof(PoolRow));
    row->block = block;
    row->stateBits = state;
    p->rows[idx] = row;

    u32 poolIdx = address >> 8 & 0x1fffff;
    u64 key = blockCacheKey(address, state);
    blockCache[key] = block;
    poolBlockKeys.insert({poolIdx, key});
    memory::jitprotect(true);
  }
  return block;
}

auto CPU::Recompiler::JITContext::update(const CPU& cpu) -> void {
  singleInstruction = GDB::server.hasBreakpoints();
  endian = Context::Endian(cpu.context.endian);
  mode = Context::Mode(cpu.context.mode);
  cop1Enabled = cpu.scc.status.enable.coprocessor1 > 0;
  floatingPointMode = cpu.scc.status.floatingPointMode > 0;
  is64bit = cpu.context.bits == 64;

  stateBits = toBits();
}

auto CPU::Recompiler::JITContext::toBits() const -> u32 {
  u32 bits = 1; // first bit always set to make 0 invalid
  bits |= singleInstruction ? 1 << 1 : 0;
  bits |= endian ? 1 << 2 : 0;
  bits |= (mode & 0x03) << 3;
  bits |= cop1Enabled ? 1 << 5 : 0;
  // bits |= floatingPointMode ? 1 << 6 : 0;
  bits |= is64bit ? 1 << 7 : 0;
  return bits;
}

#define IpuBase        offsetof(IPU, r[16])
#define IpuReg(r)      sreg(1), offsetof(IPU, r) - IpuBase
#define PipelineReg(x) mem(sreg(0), offsetof(CPU, pipeline) + offsetof(Pipeline, x))

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
auto CPU::Recompiler::emit(u64 vaddr, u32 address) -> Block* {
  if(unlikely(allocator.available() < 1_MiB)) {
    print("CPU allocator flush\n");
    allocator.release();
    reset();
  }

  // abort compilation of block asap if the instruction cache is not coherent
  if(!self.icache.coherent(vaddr, address))
    return nullptr;

  bool abort = false;

  beginFunction(3);

  Thread thread;
  bool hasBranched = 0;
  int numInsn = 0;
  constexpr u32 branchToSelf = 0x1000'ffff;  //beq 0,0,<pc>
  u32 jumpToSelf = 2 << 26 | vaddr >> 2 & 0x3ff'ffff;  //j <pc>
  while(true) {
    u32 instruction = bus.read<Word>(address, thread, "Ares Recompiler");
    mov32(PipelineReg(nstate), imm(0));
    mov64(reg(0), PipelineReg(nextpc));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    if(callInstructionPrologue) {
      callf(&CPU::instructionPrologue, imm64(vaddr), imm(instruction));
    }
    if(numInsn == 0 || (vaddr&0x1f)==0){
      //abort compilation of block if the instruction cache is not coherent
      if(!self.icache.coherent(vaddr, address)) {
        resetCompiler();
        return nullptr;
      }
      callf(&CPU::jitFetch, imm64(vaddr), imm(address));
    }
    numInsn++;
    bool branched = emitEXECUTE(instruction);
    if(unlikely(instruction == branchToSelf || instruction == jumpToSelf)) {
      //accelerate idle loops
      callf(&CPU::step, imm(64 * 2));
    } else {
      callf(&CPU::step, imm(1 * 2));
    }
    test32(PipelineReg(state), imm(Pipeline::EndBlock), set_z);
    mov32(PipelineReg(state), PipelineReg(nstate));
    mov64(mem(IpuReg(pc)), PipelineReg(pc));

    vaddr += 4;
    address += 4;
    jumpToSelf += 4;
    if(hasBranched || (address & 0xfc) == 0 || jitContext.singleInstruction) break;  //block boundary
    hasBranched = branched;
    jumpEpilog(flag_nz);
  }

  jumpEpilog();

  memory::jitprotect(false);
  auto block = (Block*)allocator.acquire(sizeof(Block));
  block->code = endFunction();

  //print(hex(PC, 8L), " ", instructions, " ", size(), "\n");
  return block;
}
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic pop
#endif

#define Sa  (instruction >>  6 & 31)
#define Rdn (instruction >> 11 & 31)
#define Rtn (instruction >> 16 & 31)
#define Rsn (instruction >> 21 & 31)
#define Fdn (instruction >>  6 & 31)
#define Fsn (instruction >> 11 & 31)
#define Ftn (instruction >> 16 & 31)

#define Rd        IpuReg(r[0]) + Rdn * sizeof(r64)
#define Rt        IpuReg(r[0]) + Rtn * sizeof(r64)
#define Rt32      IpuReg(r[0].u32) + Rtn * sizeof(r64)
#define Rs        IpuReg(r[0]) + Rsn * sizeof(r64)
#define Rs32      IpuReg(r[0].u32) + Rsn * sizeof(r64)
#define Lo        IpuReg(lo)
#define Hi        IpuReg(hi)

#define FpuBase   offsetof(FPU, r[16])
#define FpuReg(r) sreg(2), offsetof(FPU, r) - FpuBase
#define Fd        FpuReg(r[0]) + Fdn * sizeof(r64)
#define Fs        FpuReg(r[0]) + Fsn * sizeof(r64)
#define Ft        FpuReg(r[0]) + Ftn * sizeof(r64)

#define i16 s16(instruction)
#define n16 u16(instruction)
#define n26 u32(instruction & 0x03ff'ffff)

auto CPU::Recompiler::emitZeroClear(u32 n) -> void {
  if(n == 0) mov64(mem(IpuReg(r[0])), imm(0));
}

auto CPU::Recompiler::emitOverflowCheck() -> sljit_jump* {
    // If overflow flag set: throw an exception, skip the instruction via the 'end' label.
    auto didntOverflow = jump(flag_no);
    call(&CPU::arithmeticOverflowException);
    auto end = jump();
    setLabel(didntOverflow);
    return end;
}

auto CPU::Recompiler::checkDualAllowed() -> bool {
  if (jitContext.mode != Context::Mode::Kernel && !jitContext.is64bit) {
    call(&CPU::reservedInstructionException);
    return false;
  }

  return true;
}

auto CPU::Recompiler::emitEXECUTE(u32 instruction) -> bool {
  switch(instruction >> 26) {

  //SPECIAL
  case 0x00: {
    return emitSPECIAL(instruction);
  }

  //REGIMM
  case 0x01: {
    return emitREGIMM(instruction);
  }

  //J n26
  case 0x02: {
    callf(&CPU::J, imm(n26));
    return 1;
  }

  //JAL n26
  case 0x03: {
    callf(&CPU::JAL, imm(n26));
    return 1;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    callf(&CPU::BEQ, mem(Rs), mem(Rt), imm(i16));
    return 1;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    callf(&CPU::BNE, mem(Rs), mem(Rt), imm(i16));
    return 1;
  }

  //BLEZ Rs,i16
  case 0x06: {
    callf(&CPU::BLEZ, mem(Rs), imm(i16));
    return 1;
  }

  //BGTZ Rs,i16
  case 0x07: {
    callf(&CPU::BGTZ, mem(Rs), imm(i16));
    return 1;
  }

  //ADDI Rt,Rs,i16
  case 0x08: {
    callf(&CPU::ADDI, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //ADDIU Rt,Rs,i16
  case 0x09: {
    if(Rtn == 0) return 0;
    add32(reg(0), mem(Rs32), imm(i16));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rt), reg(0));
    return 0;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    if(Rtn == 0) return 0;
    cmp64(mem(Rs), imm(i16), set_slt);
    mov64_f(mem(Rt), flag_slt);
    return 0;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    if(Rtn == 0) return 0;
    cmp64(mem(Rs), imm(i16), set_ult);
    mov64_f(mem(Rt), flag_ult);
    return 0;
  }

  //ANDI Rt,Rs,n16
  case 0x0c: {
    if(Rtn == 0) return 0;
    and64(mem(Rt), mem(Rs), imm(n16));
    return 0;
  }

  //ORI Rt,Rs,n16
  case 0x0d: {
    if(Rtn == 0) return 0;
    or64(mem(Rt), mem(Rs), imm(n16));
    return 0;
  }

  //XORI Rt,Rs,n16
  case 0x0e: {
    if(Rtn == 0) return 0;
    xor64(mem(Rt), mem(Rs), imm(n16));
    return 0;
  }

  //LUI Rt,n16
  case 0x0f: {
    if(Rtn == 0) return 0;
    mov64(mem(Rt), imm(s32(n16 << 16)));
    return 0;
  }

  //SCC
  case 0x10: {
    return emitSCC(instruction);
  }

  //FPU
  case 0x11: {
    return emitFPU(instruction);
  }

  //COP2
  case 0x12: {
    return emitCOP2(instruction);
  }

  //COP3
  case 0x13: {
    callf(&CPU::COP3);
    return 1;
  }

  //BEQL Rs,Rt,i16
  case 0x14: {
    callf(&CPU::BEQL, mem(Rs), mem(Rt), imm(i16));
    return 1;
  }

  //BNEL Rs,Rt,i16
  case 0x15: {
    callf(&CPU::BNEL, mem(Rs), mem(Rt), imm(i16));
    return 1;
  }

  //BLEZL Rs,i16
  case 0x16: {
    callf(&CPU::BLEZL, mem(Rs), imm(i16));
    return 1;
  }

  //BGTZL Rs,i16
  case 0x17: {
    callf(&CPU::BGTZL, mem(Rs), imm(i16));
    return 1;
  }

  //DADDI Rt,Rs,i16
  case 0x18: {
    if (!checkDualAllowed()) return 1;
    add64(reg(0), mem(Rs), imm(i16), set_o);
    auto skip = emitOverflowCheck();
    if(Rtn > 0) mov64(mem(Rt), reg(0));
    setLabel(skip);
    return 0;
  }

  //DADDIU Rt,Rs,i16
  case 0x19: {
    if (!checkDualAllowed()) return 1;
    if(Rtn > 0) {
      add64(reg(0), mem(Rs), imm(i16));
      mov64(mem(Rt), reg(0));
    }
    return 0;
  }

  //LDL Rt,Rs,i16
  case 0x1a: {
    callf(&CPU::LDL, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LDR Rt,Rs,i16
  case 0x1b: {
    callf(&CPU::LDR, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //INVALID
  case range4(0x1c, 0x1f): {
    callf(&CPU::INVALID);
    return 1;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    callf(&CPU::LB, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    callf(&CPU::LH, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LWL Rt,Rs,i16
  case 0x22: {
    callf(&CPU::LWL, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }
  //LW Rt,Rs,i16
  case 0x23: {
    callf(&CPU::LW, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    callf(&CPU::LBU, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    callf(&CPU::LHU, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LWR Rt,Rs,i16
  case 0x26: {
    callf(&CPU::LWR, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LWU Rt,Rs,i16
  case 0x27: {
    callf(&CPU::LWU, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    callf(&CPU::SB, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    callf(&CPU::SH, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SWL Rt,Rs,i16
  case 0x2a: {
    callf(&CPU::SWL, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    callf(&CPU::SW, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SDL Rt,Rs,i16
  case 0x2c: {
    callf(&CPU::SDL, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SDR Rt,Rs,i16
  case 0x2d: {
    callf(&CPU::SDR, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SWR Rt,Rs,i16
  case 0x2e: {
    callf(&CPU::SWR, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //CACHE op(offset),base
  case 0x2f: {
    callf(&CPU::CACHE, imm(instruction >> 16 & 31), mem(Rs), imm(i16));
    return 0;
  }

  //LL Rt,Rs,i16
  case 0x30: {
    callf(&CPU::LL, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LWC1 Ft,Rs,i16
  case 0x31: {
    callf(&CPU::LWC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //LWC2
  case 0x32: {
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //LWC3
  case 0x33: {
    callf(&CPU::COP3);
    return 1;
  }

  //LLD Rt,Rs,i16
  case 0x34: {
    callf(&CPU::LLD, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LDC1 Ft,Rs,i16
  case 0x35: {
    callf(&CPU::LDC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //LDC2
  case 0x36: {
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //LD Rt,Rs,i16
  case 0x37: {
    callf(&CPU::LD, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //SC Rt,Rs,i16
  case 0x38: {
    callf(&CPU::SC, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //SWC1 Ft,Rs,i16
  case 0x39: {
    callf(&CPU::SWC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //SWC2
  case 0x3a: {
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //SWC3
  case 0x3b: {
    callf(&CPU::COP3);
    return 1;
  }

  //SCD Rt,Rs,i16
  case 0x3c: {
    callf(&CPU::SCD, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //SDC1 Ft,Rs,i16
  case 0x3d: {
    callf(&CPU::SDC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //SDC2
  case 0x3e: {
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //SD Rt,Rs,i16
  case 0x3f: {
    callf(&CPU::SD, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitSPECIAL(u32 instruction) -> bool {
  switch(instruction & 0x3f) {

  //SLL Rd,Rt,Sa
  case 0x00: {
    if(Rdn == 0) return 0;
    shl32(reg(0), mem(Rt32), imm(Sa));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //INVALID
  case 0x01: {
    callf(&CPU::INVALID);
    return 1;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    if(Rdn == 0) return 0;
    lshr32(reg(0), mem(Rt32), imm(Sa));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    if(Rdn == 0) return 0;
    ashr64(reg(0), mem(Rt), imm(Sa));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    if(Rdn == 0) return 0;
    mshl32(reg(0), mem(Rt32), mem(Rs32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //INVALID
  case 0x05: {
    callf(&CPU::INVALID);
    return 1;
  }

  //SRLV Rd,Rt,RS
  case 0x06: {
    if(Rdn == 0) return 0;
    mlshr32(reg(0), mem(Rt32), mem(Rs32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    if(Rdn == 0) return 0;
    and64(reg(1), mem(Rs), imm(31));
    ashr64(reg(0), mem(Rt), reg(1));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //JR Rs
  case 0x08: {
    callf(&CPU::JR, mem(Rs));
    return 1;
  }

  //JALR Rd,Rs
  case 0x09: {
    callf(&CPU::JALR, mem(Rd), mem(Rs));
    emitZeroClear(Rdn);
    return 1;
  }

  //INVALID
  case range2(0x0a, 0x0b): {
    callf(&CPU::INVALID);
    return 1;
  }

  //SYSCALL
  case 0x0c: {
    callf(&CPU::SYSCALL);
    return 1;
  }

  //BREAK
  case 0x0d: {
    callf(&CPU::BREAK);
    return 1;
  }

  //INVALID
  case 0x0e: {
    callf(&CPU::INVALID);
    return 1;
  }

  //SYNC
  case 0x0f: {
    callf(&CPU::SYNC);
    return 0;
  }

  //MFHI Rd
  case 0x10: {
    if(Rdn == 0) return 0;
    mov64(mem(Rd), mem(Hi));
    return 0;
  }

  //MTHI Rs
  case 0x11: {
    mov64(mem(Hi), mem(Rs));
    return 0;
  }

  //MFLO Rd
  case 0x12: {
    if(Rdn == 0) return 0;
    mov64(mem(Rd), mem(Lo));
    return 0;
  }

  //MTLO Rs
  case 0x13: {
    mov64(mem(Lo), mem(Rs));
    return 0;
  }

  //DSLLV Rd,Rt,Rs
  case 0x14: {
    if (!checkDualAllowed()) return 1;
    if (Rdn == 0) return 0;
    and64(reg(0), mem(Rs32), imm(63));
    shl64(mem(Rd), mem(Rt), reg(0));
    return 0;
  }

  //INVALID
  case 0x15: {
    callf(&CPU::INVALID);
    return 1;
  }

  //DSRLV Rd,Rt,Rs
  case 0x16: {
    if (!checkDualAllowed()) return 1;
    if (Rdn == 0) return 0;
    and64(reg(0), mem(Rs32), imm(63));
    lshr64(mem(Rd), mem(Rt), reg(0));
    return 0;
  }

  //DSRAV Rd,Rt,Rs
  case 0x17: {
    if (!checkDualAllowed()) return 1;
    if (Rdn == 0) return 0;
    and64(reg(0), mem(Rs32), imm(63));
    ashr64(mem(Rd), mem(Rt), reg(0));
    return 0;
  }

  //MULT Rs,Rt
  case 0x18: {
    callf(&CPU::MULT, mem(Rs), mem(Rt));
    return 0;
  }

  //MULTU Rs,Rt
  case 0x19: {
    callf(&CPU::MULTU, mem(Rs), mem(Rt));
    return 0;
  }

  //DIV Rs,Rt
  case 0x1a: {
    callf(&CPU::DIV, mem(Rs), mem(Rt));
    return 0;
  }

  //DIVU Rs,Rt
  case 0x1b: {
    callf(&CPU::DIVU, mem(Rs), mem(Rt));
    return 0;
  }

  //DMULT Rs,Rt
  case 0x1c: {
    callf(&CPU::DMULT, mem(Rs), mem(Rt));
    return 0;
  }

  //DMULTU Rs,Rt
  case 0x1d: {
    callf(&CPU::DMULTU, mem(Rs), mem(Rt));
    return 0;
  }

  //DDIV Rs,Rt
  case 0x1e: {
    callf(&CPU::DDIV, mem(Rs), mem(Rt));
    return 0;
  }

  //DDIVU Rs,Rt
  case 0x1f: {
    callf(&CPU::DDIVU, mem(Rs), mem(Rt));
    return 0;
  }

  //ADD Rd,Rs,Rt
  case 0x20: {
    callf(&CPU::ADD, mem(Rd), mem(Rs), mem(Rt));
    emitZeroClear(Rdn);
    return 0;
  }

  //ADDU Rd,Rs,Rt
  case 0x21: {
    if(Rdn == 0) return 0;
    add32(reg(0), mem(Rs32), mem(Rt32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //SUB Rd,Rs,Rt
  case 0x22: {
    callf(&CPU::SUB, mem(Rd), mem(Rs), mem(Rt));
    emitZeroClear(Rdn);
    return 0;
  }

  //SUBU Rd,Rs,Rt
  case 0x23: {
    if(Rdn == 0) return 0;
    sub32(reg(0), mem(Rs32), mem(Rt32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    if(Rdn == 0) return 0;
    and64(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    if(Rdn == 0) return 0;
    or64(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    if(Rdn == 0) return 0;
    xor64(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    if(Rdn == 0) return 0;
    or64(reg(0), mem(Rs), mem(Rt));
    xor64(reg(0), reg(0), imm(-1));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //INVALID
  case range2(0x28, 0x29): {
    callf(&CPU::INVALID);
    return 1;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    if(Rdn == 0) return 0;
    cmp64(mem(Rs), mem(Rt), set_slt);
    mov64_f(mem(Rd), flag_slt);
    return 0;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    if(Rdn == 0) return 0;
    cmp64(mem(Rs), mem(Rt), set_ult);
    mov64_f(mem(Rd), flag_ult);
    return 0;
  }

  //DADD Rd,Rs,Rt
  case 0x2c: {
    if (!checkDualAllowed()) return 1;
    add64(reg(0), mem(Rs), mem(Rt), set_o);
    auto skip = emitOverflowCheck();
    if(Rdn > 0) mov64(mem(Rd), reg(0));
    setLabel(skip);
    return 0;
  }

  //DADDU Rd,Rs,Rt
  case 0x2d: {
    if (!checkDualAllowed()) {
      return 1;
    }

    if(Rdn == 0) return 0;

    add64(reg(0), mem(Rs), mem(Rt));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //DSUB Rd,Rs,Rt
  case 0x2e: {
    if (!checkDualAllowed()) return 1;
    sub64(reg(0), mem(Rs), mem(Rt), set_o);
    auto skip = emitOverflowCheck();
    if(Rdn > 0) mov64(mem(Rd), reg(0));
    setLabel(skip);
    return 0;
  }

  //DSUBU Rd,Rs,Rt
  case 0x2f: {
    if (!checkDualAllowed()) return 1;
    if(Rdn > 0) {
      sub64(reg(0), mem(Rs), mem(Rt));
      mov64(mem(Rd), reg(0));
    }
    return 0;
  }

  //TGE Rs,Rt
  case 0x30: {
    callf(&CPU::TGE, mem(Rs), mem(Rt));
    return 0;
  }

  //TGEU Rs,Rt
  case 0x31: {
    callf(&CPU::TGEU, mem(Rs), mem(Rt));
    return 0;
  }

  //TLT Rs,Rt
  case 0x32: {
    callf(&CPU::TLT, mem(Rs), mem(Rt));
    return 0;
  }

  //TLTU Rs,Rt
  case 0x33: {
    callf(&CPU::TLTU, mem(Rs), mem(Rt));
    return 0;
  }

  //TEQ Rs,Rt
  case 0x34: {
    callf(&CPU::TEQ, mem(Rs), mem(Rt));
    return 0;
  }

  //INVALID
  case 0x35: {
    callf(&CPU::INVALID);
    return 1;
  }

  //TNE Rs,Rt
  case 0x36: {
    callf(&CPU::TNE, mem(Rs), mem(Rt));
    return 0;
  }

  //INVALID
  case 0x37: {
    callf(&CPU::INVALID);
    return 1;
  }
  //DSLL Rd,Rt,Sa
  case 0x38: {
    if (!checkDualAllowed()) return 1;
    if (Rdn == 0) return 0;
    shl64(mem(Rd), mem(Rt), imm(Sa));
    return 0;
  }

  //INVALID
  case 0x39: {
    callf(&CPU::INVALID);
    return 1;
  }

  //DSRL Rd,Rt,Sa
  case 0x3a: {
    callf(&CPU::DSRL, mem(Rd), mem(Rt), imm(Sa));
    emitZeroClear(Rdn);
    return 0;
  }

  //DSRA Rd,Rt,Sa
  case 0x3b: {
    if (!checkDualAllowed()) return 1;
    if (Rdn == 0) return 0;
    ashr64(mem(Rd), mem(Rt), imm(Sa));
    return 0;
  }

  //DSLL32 Rd,Rt,Sa
  case 0x3c: {
    if (!checkDualAllowed()) return 1;
    if (Rdn == 0) return 0;
    shl64(mem(Rd), mem(Rt), imm(Sa+32));
    return 0;
  }

  //INVALID
  case 0x3d: {
    callf(&CPU::INVALID);
    return 1;
  }

  //DSRL32 Rd,Rt,Sa
  case 0x3e: {
    callf(&CPU::DSRL, mem(Rd), mem(Rt), imm(Sa+32));
    emitZeroClear(Rdn);
    return 0;
  }

  //DSRA32 Rd,Rt,Sa
  case 0x3f: {
    callf(&CPU::DSRA, mem(Rd), mem(Rt), imm(Sa+32));
    emitZeroClear(Rdn);
    return 0;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitREGIMM(u32 instruction) -> bool {
  switch(instruction >> 16 & 0x1f) {

  //BLTZ Rs,i16
  case 0x00: {
    callf(&CPU::BLTZ, mem(Rs), imm(i16));
    return 0;
  }

  //BGEZ Rs,i16
  case 0x01: {
    callf(&CPU::BGEZ, mem(Rs), imm(i16));
    return 0;
  }

  //BLTZL Rs,i16
  case 0x02: {
    callf(&CPU::BLTZL, mem(Rs), imm(i16));
    return 0;
  }

  //BGEZL Rs,i16
  case 0x03: {
    callf(&CPU::BGEZL, mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case range4(0x04, 0x07): {
    call(&CPU::INVALID);
    return 1;
  }

  //TGEI Rs,i16
  case 0x08: {
    callf(&CPU::TGEI, mem(Rs), imm(i16));
    return 0;
  }

  //TGEIU Rs,i16
  case 0x09: {
    callf(&CPU::TGEIU, mem(Rs), imm(i16));
    return 0;
  }

  //TLTI Rs,i16
  case 0x0a: {
    callf(&CPU::TLTI, mem(Rs), imm(i16));
    return 0;
  }

  //TLTIU Rs,i16
  case 0x0b: {
    callf(&CPU::TLTIU, mem(Rs), imm(i16));
    return 0;
  }

  //TEQI Rs,i16
  case 0x0c: {
    callf(&CPU::TEQI, mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case 0x0d: {
    callf(&CPU::INVALID);
    return 1;
  }

  //TNEI Rs,i16
  case 0x0e: {
    callf(&CPU::TNEI, mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case 0x0f: {
    callf(&CPU::INVALID);
    return 1;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    callf(&CPU::BLTZAL, mem(Rs), imm(i16));
    return 0;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    callf(&CPU::BGEZAL, mem(Rs), imm(i16));
    return 0;
  }

  //BLTZALL Rs,i16
  case 0x12: {
    callf(&CPU::BLTZALL, mem(Rs), imm(i16));
    return 0;
  }

  //BGEZALL Rs,i16
  case 0x13: {
    callf(&CPU::BGEZALL, mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case range12(0x14, 0x1f): {
    callf(&CPU::INVALID);
    return 1;
  }
  }

  return 0;
}

auto CPU::Recompiler::emitSCC(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

//MFC0 Rt,Rd
  case 0x00: {
    callf(&CPU::MFC0, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //DMFC0 Rt,Rd
  case 0x01: {
    callf(&CPU::DMFC0, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //INVALID
  case range2(0x02, 0x03): {
    callf(&CPU::INVALID);
    return 1;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    callf(&CPU::MTC0, mem(Rt), imm(Rdn));
    return 0;
  }

  //DMTC0 Rt,Rd
  case 0x05: {
    callf(&CPU::DMTC0, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case range10(0x06, 0x0f): {
    callf(&CPU::INVALID);
    return 1;
  }

  }

  switch(instruction & 0x3f) {

  //TLBR
  case 0x01: {
    callf(&CPU::TLBR);
    return 0;
  }

  //TLBWI
  case 0x02: {
    callf(&CPU::TLBWI);
    return 0;
  }

  //TLBWR
  case 0x06: {
    callf(&CPU::TLBWR);
    return 0;
  }

  //TLBP
  case 0x08: {
    callf(&CPU::TLBP);
    return 0;
  }

  //ERET
  case 0x18: {
    callf(&CPU::ERET);
    return 1;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitFPU(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

  //MFC1 Rt,Fs
  case 0x00: {
    callf(&CPU::MFC1, mem(Rt), imm(Fsn));
    emitZeroClear(Rtn);
    return 0;
  }

  //DMFC1 Rt,Fs
  case 0x01: {
    callf(&CPU::DMFC1, mem(Rt), imm(Fsn));
    emitZeroClear(Rtn);
    return 0;
  }

  //CFC1 Rt,Rd
  case 0x02: {
    callf(&CPU::CFC1, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //DCFC1 Rt,Rd
  case 0x03: {
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //MTC1 Rt,Fs
  case 0x04: {
    callf(&CPU::MTC1, mem(Rt), imm(Fsn));
    return 0;
  }

  //DMTC1 Rt,Fs
  case 0x05: {
    callf(&CPU::DMTC1, mem(Rt), imm(Fsn));
    return 0;
  }

  //CTC1 Rt,Rd
  case 0x06: {
    callf(&CPU::CTC1, mem(Rt), imm(Rdn));
    return 0;
  }

  //DCTC1 Rt,Rd
  case 0x07: {
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //BC1 offset
  case 0x08: {
    callf(&CPU::BC1, imm(instruction >> 16 & 1), imm(instruction >> 17 & 1), imm(i16));
    return 1;
  }

  //INVALID
  case range7(0x09, 0x0f): {
    callf(&CPU::INVALID);
    return 1;
  }

  }

  if((instruction >> 21 & 31) == 16)
  switch(instruction & 0x3f) {

  //FADD.S Fd,Fs,Ft
  case 0x00: {
    callf(&CPU::FADD_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSUB.S Fd,Fs,Ft
  case 0x01: {
    callf(&CPU::FSUB_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FMUL.S Fd,Fs,Ft
  case 0x02: {
    callf(&CPU::FMUL_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FDIV.S Fd,Fs,Ft
  case 0x03: {
    callf(&CPU::FDIV_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSQRT.S Fd,Fs
  case 0x04: {
    callf(&CPU::FSQRT_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FABS.S Fd,Fs
  case 0x05: {
    callf(&CPU::FABS_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FMOV.S Fd,Fs
  case 0x06: {
    callf(&CPU::FMOV_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FNEG.S Fd,Fs
  case 0x07: {
    callf(&CPU::FNEG_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FROUND.L.S Fd,Fs
  case 0x08: {
    callf(&CPU::FROUND_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.L.S Fd,Fs
  case 0x09: {
    callf(&CPU::FTRUNC_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.L.S Fd,Fs
  case 0x0a: {
    callf(&CPU::FCEIL_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.L.S Fd,Fs
  case 0x0b: {
    callf(&CPU::FFLOOR_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FROUND.W.S Fd,Fs
  case 0x0c: {
    callf(&CPU::FROUND_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.W.S Fd,Fs
  case 0x0d: {
    callf(&CPU::FTRUNC_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.W.S Fd,Fs
  case 0x0e: {
    callf(&CPU::FCEIL_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.W.S Fd,Fs
  case 0x0f: {
    callf(&CPU::FFLOOR_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.S.S Fd,Fs
  case 0x20: {
    callf(&CPU::FCVT_S_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.S Fd,Fs
  case 0x21: {
    callf(&CPU::FCVT_D_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.W.S Fd,Fs
  case 0x24: {
    callf(&CPU::FCVT_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.L.S Fd,Fs
  case 0x25: {
    callf(&CPU::FCVT_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FC.F.S Fs,Ft
  case 0x30: {
    callf(&CPU::FC_F_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UN.S Fs,Ft
  case 0x31: {
    callf(&CPU::FC_UN_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.EQ.S Fs,Ft
  case 0x32: {
    callf(&CPU::FC_EQ_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UEQ.S Fs,Ft
  case 0x33: {
    callf(&CPU::FC_UEQ_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLT.S Fs,Ft
  case 0x34: {
    callf(&CPU::FC_OLT_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULT.S Fs,Ft
  case 0x35: {
    callf(&CPU::FC_ULT_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLE.S Fs,Ft
  case 0x36: {
    callf(&CPU::FC_OLE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULE.S Fs,Ft
  case 0x37: {
    callf(&CPU::FC_ULE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SF.S Fs,Ft
  case 0x38: {
    callf(&CPU::FC_SF_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGLE.S Fs,Ft
  case 0x39: {
    callf(&CPU::FC_NGLE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SEQ.S Fs,Ft
  case 0x3a: {
    callf(&CPU::FC_SEQ_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGL.S Fs,Ft
  case 0x3b: {
    callf(&CPU::FC_NGL_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LT.S Fs,Ft
  case 0x3c: {
    callf(&CPU::FC_LT_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGE.S Fs,Ft
  case 0x3d: {
    callf(&CPU::FC_NGE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LE.S Fs,Ft
  case 0x3e: {
    callf(&CPU::FC_LE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGT.S Fs,Ft
  case 0x3f: {
    callf(&CPU::FC_NGT_S, imm(Fsn), imm(Ftn));
    return 0;
  }
  }

  if((instruction >> 21 & 31) == 17)
  switch(instruction & 0x3f) {

//FADD.D Fd,Fs,Ft
  case 0x00: {
    callf(&CPU::FADD_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSUB.D Fd,Fs,Ft
  case 0x01: {
    callf(&CPU::FSUB_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FMUL.D Fd,Fs,Ft
  case 0x02: {
    callf(&CPU::FMUL_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FDIV.D Fd,Fs,Ft
  case 0x03: {
    callf(&CPU::FDIV_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSQRT.D Fd,Fs
  case 0x04: {
    callf(&CPU::FSQRT_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FABS.D Fd,Fs
  case 0x05: {
    callf(&CPU::FABS_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FMOV.D Fd,Fs
  case 0x06: {
    callf(&CPU::FMOV_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FNEG.D Fd,Fs
  case 0x07: {
    callf(&CPU::FNEG_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FROUND.L.D Fd,Fs
  case 0x08: {
    callf(&CPU::FROUND_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.L.D Fd,Fs
  case 0x09: {
    callf(&CPU::FTRUNC_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.L.D Fd,Fs
  case 0x0a: {
    callf(&CPU::FCEIL_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.L.D Fd,Fs
  case 0x0b: {
    callf(&CPU::FFLOOR_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FROUND.W.D Fd,Fs
  case 0x0c: {
    callf(&CPU::FROUND_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.W.D Fd,Fs
  case 0x0d: {
    callf(&CPU::FTRUNC_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.W.D Fd,Fs
  case 0x0e: {
    callf(&CPU::FCEIL_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.W.D Fd,Fs
  case 0x0f: {
    callf(&CPU::FFLOOR_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.S.D Fd,Fs
  case 0x20: {
    callf(&CPU::FCVT_S_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.D Fd,Fs
  case 0x21: {
    callf(&CPU::FCVT_D_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.W.D Fd,Fs
  case 0x24: {
    callf(&CPU::FCVT_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.L.D Fd,Fs
  case 0x25: {
    callf(&CPU::FCVT_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FC.F.D Fs,Ft
  case 0x30: {
    callf(&CPU::FC_F_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UN.D Fs,Ft
  case 0x31: {
    callf(&CPU::FC_UN_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.EQ.D Fs,Ft
  case 0x32: {
    callf(&CPU::FC_EQ_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UEQ.D Fs,Ft
  case 0x33: {
    callf(&CPU::FC_UEQ_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLT.D Fs,Ft
  case 0x34: {
    callf(&CPU::FC_OLT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULT.D Fs,Ft
  case 0x35: {
    callf(&CPU::FC_ULT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLE.D Fs,Ft
  case 0x36: {
    callf(&CPU::FC_OLE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULE.D Fs,Ft
  case 0x37: {
    callf(&CPU::FC_ULE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SF.D Fs,Ft
  case 0x38: {
    callf(&CPU::FC_SF_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGLE.D Fs,Ft
  case 0x39: {
    callf(&CPU::FC_NGLE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SEQ.D Fs,Ft
  case 0x3a: {
    callf(&CPU::FC_SEQ_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGL.D Fs,Ft
  case 0x3b: {
    callf(&CPU::FC_NGL_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LT.D Fs,Ft
  case 0x3c: {
    callf(&CPU::FC_LT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGE.D Fs,Ft
  case 0x3d: {
    callf(&CPU::FC_NGE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LE.D Fs,Ft
  case 0x3e: {
    callf(&CPU::FC_LE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGT.D Fs,Ft
  case 0x3f: {
    callf(&CPU::FC_NGT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  }

  if((instruction >> 21 & 31) == 20)
  switch(instruction & 0x3f) {
  case range8(0x08, 0x0f): {
    call(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  case range2(0x24, 0x25): {
    call(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //FCVT.S.W Fd,Fs
  case 0x20: {
    callf(&CPU::FCVT_S_W, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.W Fd,Fs
  case 0x21: {
    callf(&CPU::FCVT_D_W, imm(Fdn), imm(Fsn));
    return 0;
  }

  }

  if((instruction >> 21 & 31) == 21)
  switch(instruction & 0x3f) {
  case range8(0x08, 0x0f): {
    call(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }
  case range2(0x24, 0x25): {
    call(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //FCVT.S.L
  case 0x20: {
    callf(&CPU::FCVT_S_L, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.L
  case 0x21: {
    callf(&CPU::FCVT_D_L, imm(Fdn), imm(Fsn));
    return 0;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitCOP2(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

  //MFC2 Rt,Rd
  case 0x00: {
    callf(&CPU::MFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //DMFC2 Rt,Rd
  case 0x01: {
    callf(&CPU::DMFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    callf(&CPU::CFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //INVALID
  case 0x03: {
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    callf(&CPU::MTC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //DMTC2 Rt,Rd
  case 0x05: {
    callf(&CPU::DMTC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    callf(&CPU::CTC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case range9(0x07, 0x0f): {
    callf(&CPU::COP2INVALID);
    return 1;
  }

  }
  return 0;
}

#undef IpuBase
#undef IpuReg
#undef PipelineReg
#undef Sa
#undef Rdn
#undef Rtn
#undef Rsn
#undef Fdn
#undef Fsn
#undef Ftn
#undef Rd
#undef Rt
#undef Rt32
#undef Rs
#undef Rs32
#undef Lo
#undef Hi
#undef FpuBase
#undef FpuReg
#undef Fd
#undef Fs
#undef Ft
#undef i16
#undef n16
#undef n26
