#define Sa  (instruction >>  6 & 31)
#define Rdn (instruction >> 11 & 31)
#define Rtn (instruction >> 16 & 31)
#define Rsn (instruction >> 21 & 31)
#define Fdn (instruction >>  6 & 31)
#define Fsn (instruction >> 11 & 31)
#define Ftn (instruction >> 16 & 31)
#define XRtn   (instruction >> 15 & 31)
#define XRdn   (instruction >> 20 & 31)
#define XCODE  (instruction >> 6  & 511)

#define Rd        IpuReg(r[0]) + Rdn * sizeof(r64)
#define Rt        IpuReg(r[0]) + Rtn * sizeof(r64)
#define Rt32      IpuReg(r[0].u32) + Rtn * sizeof(r64)
#define Rs        IpuReg(r[0]) + Rsn * sizeof(r64)
#define Rs32      IpuReg(r[0].u32) + Rsn * sizeof(r64)
#define Lo        IpuReg(lo)
#define Hi        IpuReg(hi)

#define XRd       IpuReg(r[0]) + XRdn * sizeof(r64)
#define XRt       IpuReg(r[0]) + XRtn * sizeof(r64)

#define i16       s16(instruction)
#define n16       u16(instruction)
#define n26       u32(instruction & 0x03ff'ffff)

#define FpuBase   offsetof(FPU, r[16])
#define FpuReg(r) sreg(2), offsetof(FPU, r) - FpuBase
#define Fd        FpuReg(r[0]) + Fdn * sizeof(r64)
#define Fs        FpuReg(r[0]) + Fsn * sizeof(r64)
#define Ft        FpuReg(r[0]) + Ftn * sizeof(r64)
static constexpr s32 FpuCsrBaseOffset = offsetof(CPU, fpu) + offsetof(CPU::FPU, csr);
static constexpr s32 FpuCsrFlagDataOffset = FpuCsrBaseOffset
                                          + offsetof(CPU::FPU::ControlStatus, flag)
                                          + offsetof(CPU::FPU::ControlStatus::Flag, data);
static constexpr s32 FpuCsrCauseOffset = FpuCsrBaseOffset + offsetof(CPU::FPU::ControlStatus, cause);
static constexpr s32 FpuCsrCauseDataOffset = FpuCsrCauseOffset + offsetof(CPU::FPU::ControlStatus::Cause, data);
static constexpr s32 FpuR64S32Off  = offsetof(CPU::r64, s32);
static constexpr s32 FpuR64S32hOff = offsetof(CPU::r64, s32h);
static constexpr s32 RecompilerBaseOffset = offsetof(CPU, recompiler);
static constexpr s32 RecompilerFpuFastMxcsrOffset = RecompilerBaseOffset + offsetof(CPU::Recompiler, emitFpuFastMxcsr);
static constexpr s32 RecompilerFpuSaveMxcsrOffset = RecompilerBaseOffset + offsetof(CPU::Recompiler, emitFpuSaveMxcsr);
#define FpuCsrCompare mem(sreg(0), FpuCsrBaseOffset + offsetof(CPU::FPU::ControlStatus, compare))

static_assert(sizeof(n1) == 1);
static_assert(sizeof(CPU::FPU::ControlStatus::Flag) == 1);
static_assert(sizeof(CPU::FPU::ControlStatus::Enable) == 1);
static_assert(sizeof(CPU::FPU::ControlStatus::Cause) == 1);
static_assert(offsetof(CPU::FPU::ControlStatus::Flag, data) == 0);
static_assert(offsetof(CPU::FPU::ControlStatus::Enable, data) == 0);
static_assert(offsetof(CPU::FPU::ControlStatus::Cause, data) == 0);
static_assert(CPU::FPU::ControlStatus::InvalidOperationBit == 0);
#if defined(ARCHITECTURE_ARM64)
static_assert(CPU::FPU::ControlStatus::DivisionByZeroBit == 1);
static_assert(CPU::FPU::ControlStatus::OverflowBit == 2);
static_assert(CPU::FPU::ControlStatus::UnderflowBit == 3);
static_assert(CPU::FPU::ControlStatus::InexactBit == 4);
static_assert(CPU::FPU::ControlStatus::DenormalBit == 7);
#else
static_assert(CPU::FPU::ControlStatus::DenormalBit == 1);
static_assert(CPU::FPU::ControlStatus::DivisionByZeroBit == 2);
static_assert(CPU::FPU::ControlStatus::OverflowBit == 3);
static_assert(CPU::FPU::ControlStatus::UnderflowBit == 4);
static_assert(CPU::FPU::ControlStatus::InexactBit == 5);
#endif
static_assert((RecompilerFpuFastMxcsrOffset & 3) == 0);
static_assert((RecompilerFpuSaveMxcsrOffset & 3) == 0);

auto CPU::Recompiler::emitFPU(u32 instruction, EmitPcMode pcMode) -> EmitExecuteResult {
  auto movzeron = [&](s32 offset, u32 size) -> void {
    if constexpr(sizeof(void*) == 8) {
      while(size >= 8) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(sreg(0).fst), offset, SLJIT_IMM, 0);
        offset += 8;
        size -= 8;
      }
    }
    if(size >= 4) {
      sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(sreg(0).fst), offset, SLJIT_IMM, 0);
      offset += 4;
      size -= 4;
    }
    if(size >= 2) {
      sljit_emit_op1(compiler, SLJIT_MOV_U16, SLJIT_MEM1(sreg(0).fst), offset, SLJIT_IMM, 0);
      offset += 2;
      size -= 2;
    }
    if(size) {
      sljit_emit_op1(compiler, SLJIT_MOV_U8, SLJIT_MEM1(sreg(0).fst), offset, SLJIT_IMM, 0);
    }
  };
#if defined(ARCHITECTURE_ARM64) || defined(ARCHITECTURE_AMD64)
#if defined(ARCHITECTURE_ARM64)
  auto arm64RoundModeBits = [&]() -> u32 {
    switch(emitStateKey.fpuRoundMode()) {
    case 0: return 0x0000'0000u;
    case 1: return 0x00c0'0000u;
    case 2: return 0x0040'0000u;
    case 3: return 0x0080'0000u;
    }
    unreachable;
  };
  // FPCR value to be used during fast path execution
  auto arm64FpuFastFpcr = [&]() -> u32 {
    return 0x0100'0000u | arm64RoundModeBits();
  };
#elif defined(ARCHITECTURE_AMD64)
  auto amd64RoundModeBits = [&]() -> u32 {
    switch(emitStateKey.fpuRoundMode()) {
    case 0: return 0x0000'0000u;
    case 1: return 0x0000'6000u;
    case 2: return 0x0000'4000u;
    case 3: return 0x0000'2000u;
    }
    unreachable;
  };
  // MXCSR value to be used during fast path execution
  auto amd64FpuFastMxcsr = [&]() -> u32 {
    return 0x0000'9f80u | amd64RoundModeBits();
  };
  auto amd64FpuOpMxcsr = [](u32 rmode) -> u32 {
    u32 b;
    switch(rmode & 3u) {
    case 0: b = 0x0000'0000u; break;
    case 1: b = 0x0000'6000u; break;
    case 2: b = 0x0000'4000u; break;
    case 3: b = 0x0000'2000u; break;
    }
    return 0x0000'9f80u | b;
  };
#endif
  enum FpuInputCheck : u32 {
    FpuCheckNone = 0,
    FpuCheckQnan = 1 << 0,
    FpuCheckSnan = 1 << 1,
    FpuCheckSubnormal = 1 << 2,
  };
  enum FpuWidth : u32 {
    FpuWidth32 = 0,
    FpuWidth64 = 1,
  };
  enum FpuConvertType : u32 {
    FpuConvertF32 = 0,
    FpuConvertF64 = 1,
    FpuConvertS32 = 2,
    FpuConvertS64 = 3,
  };
  enum FpuCompareNanPolicy : u32 {
    FpuCompareOrdered = 0,
    FpuCompareUnordered = 1,
  };
  enum FpuCompareKind : u32 {
    FpuCompareFalse = 0,
    FpuCompareEq = 1,
    FpuCompareLt = 2,
    FpuCompareLe = 3,
  };

  constexpr u32 fpuInvalidMask    = 1u << CPU::FPU::ControlStatus::InvalidOperationBit;
  constexpr u32 fpuDiv0Mask       = 1u << CPU::FPU::ControlStatus::DivisionByZeroBit;
  constexpr u32 fpuOverflowMask   = 1u << CPU::FPU::ControlStatus::OverflowBit;
  constexpr u32 fpuUnderflowMask  = 1u << CPU::FPU::ControlStatus::UnderflowBit;
  constexpr u32 fpuInexactMask    = 1u << CPU::FPU::ControlStatus::InexactBit;
  constexpr u32 fpuDenormalMask   = 1u << CPU::FPU::ControlStatus::DenormalBit;
  constexpr u32 fpuIeeeMask       = fpuInvalidMask | fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask;

    // Sanitize a difference between VR4300 and AMD64/ARM64. In VR4300, when an underflow occurs and
    // subnormals are flushed, the resulting number is rounded according to the rounding mode. So it
    // can be either +0/-0, or the smallest-magnitude positive/negative number. Instead, on
    // AMD64/ARM64, the resulting number is always +0/-0.
    auto emitFpuUnderflowFlushFixup = [&](reg status, reg value, bool resultIsDouble) -> void {
      if(emitStateKey.fpuRoundMode() != 2 && emitStateKey.fpuRoundMode() != 3) return;
      test32(status, imm(fpuUnderflowMask), set_z);
      auto noUnderflow = jump(flag_z);
      switch(emitStateKey.fpuRoundMode()) {
      case 2: {
        if(resultIsDouble) {
          cmp64(value, imm(0), set_slt);
          mov64(value, imm(-0x8000'0000'0000'0000ll));
          mov64(reg(2), imm(0x0010'0000'0000'0000ull));
          cmov64(value, reg(2), value, flag_sge);
        } else {
          cmp32(value, imm(0), set_slt);
          mov32(value, imm(0x8000'0000u));
          mov32(reg(2), imm(0x0080'0000u));
          cmov32(value, reg(2), value, flag_sge);
        }
        break;
      }
      case 3: {
        if(resultIsDouble) {
          cmp64(value, imm(0), set_sge);
          mov64(value, imm(-0x7ff0'0000'0000'0000ll));
          mov64(reg(2), imm(0));
          cmov64(value, reg(2), value, flag_sge);
        } else {
          cmp32(value, imm(0), set_slt);
          mov32(value, imm(0x8080'0000u));
          mov32(reg(2), imm(0x0000'0000u));
          cmov32(value, reg(2), value, flag_sge);
        }
        break;
      }
      }
      setLabel(noUnderflow);
    };

  // Helper function to emit a FPU opcode.
  // The main idea is to run an equivalent FPU opcode directly on the host FPU,
  // with the same control configuration as the MIPS code. After running the
  // opcode, control flags are collected. Since VR4300 was "mostly" following
  // the same IEEE 754-1985 standard as modern CPUs, the flags will be very very
  // close. 
  // If the VR4300 was configured to raise exceptions for certain conditions,
  // and those conditions were signaled in the control flags, we just go into a
  // slow path that calls into the interpreter.
  // Otherwise, we complete the fast path by mapping the host FPU flags into the
  // VR4300 control registers, handling a few semantic differences, and write the
  // result into the destination register.
  auto emitFpuOpcode = [&](
    auto&& callSlowPath,                // Function pointer to call into the interpreter for slow path
    u32 fdn,                            // Destination register
    std::initializer_list<u32> inputs,  // List of source registers
    u32 width,                          // Operand width
    u32 cycles,                         // Cycles that the opcode takes to execute
    u32 inputChecks,                    // Bitmask of checks to perform on input operand(s)
    u32 passthroughMask,                // Bitmask of host FPU flags that can be safely masked to VR4300 FPU flags
    auto&& emitHostOpcode               // Function pointer to emit the host FPU opcode
  ) -> EmitExecuteResult {
#if !defined(ARCHITECTURE_ARM64) && !defined(ARCHITECTURE_AMD64)
    return callSlowPath();
#endif
    if(emitSlowPathSection || !emitStateKey.coprocessor1Enabled()) {
      return callSlowPath();
    }

    u32 inputCount = (u32)inputs.size();
    assert(inputCount == 1 || inputCount == 2);
    assert(width == FpuWidth32 || width == FpuWidth64);
    bool isDouble = width == FpuWidth64;

    // Decode opcode input fields into actual register numbers (using also
    // the FR bit that determines register layout).
    auto input = inputs.begin();
    u32 fsn = *input++;
    u32 ftn = inputCount == 2 ? *input : 0;
    s32 fsWordOff;
    if(isDouble) {
      if(emitStateKey.floatingPointMode()) fsWordOff = (fsn - 16) * 8;
      else fsWordOff = ((fsn & ~1) - 16) * 8;
    } else {
      if(emitStateKey.floatingPointMode()) fsWordOff = (fsn - 16) * 8 + FpuR64S32Off;
      else fsWordOff = ((fsn & ~1) - 16) * 8 + FpuR64S32Off;
    }
    s32 ftWordOff = 0;
    if(inputCount == 2) {
      if(isDouble) ftWordOff = (ftn - 16) * 8;
      else ftWordOff = (ftn - 16) * 8 + FpuR64S32Off;
    }
    s32 fdWordOff;
    if(isDouble) fdWordOff = (fdn - 16) * 8;
    else fdWordOff = (fdn - 16) * 8 + FpuR64S32Off;
    s32 fdWordhOff = (fdn - 16) * 8 + FpuR64S32hOff;

    // EMIT: read input(s)
    if(isDouble) {
      mov64(reg(0), mem(sreg(2), fsWordOff));
      if(inputCount == 2) mov64(reg(1), mem(sreg(2), ftWordOff));
    } else {
      mov32(reg(0), mem(sreg(2), fsWordOff));
      if(inputCount == 2) mov32(reg(1), mem(sreg(2), ftWordOff));
    }

    // Implement the requested input checks. VR4300 has some unimplemented cases
    // (whose list depends on the opcode), that must be explicitly checked here
    // because our host FPU would instead handle them just fine.
    // If we meet one of these conditions, we jump to the slow path.
    bool checkQnan      = inputChecks & FpuCheckQnan;
    bool checkSnan      = inputChecks & FpuCheckSnan;
    bool checkSubnormal = inputChecks & FpuCheckSubnormal;
    auto emitInputChecks = [&](reg source, sljit_jump*& qnan, sljit_jump*& snan, sljit_jump*& subnormal) -> void {
      if (inputChecks == 0) return;
      if(isDouble) {
        shl64(reg(2), source, imm(1));
        if(checkSubnormal) {
          sub64(reg(3), reg(2), imm(1));
          cmp64(reg(3), imm(0x001f'ffff'ffff'ffffull), set_ult);
          subnormal = jump(flag_ult);
        }
        if(checkQnan && !checkSnan) {
          cmp64(reg(2), imm(0xfff0'0000'0000'0000ull), set_uge);
          qnan = jump(flag_uge);
        } else if(!checkQnan && checkSnan) {
          xor64(reg(3), reg(2), imm(0x0008'0000'0000'0000ull));
          cmp64(reg(3), imm(0xfff0'0000'0000'0000ull), set_ugt);
          snan = jump(flag_ugt);
        } else if(checkQnan && checkSnan) {
          cmp64(reg(2), imm(0xffe0'0000'0000'0000ull), set_ugt);
          snan = jump(flag_ugt);
        }
      } else {
        shl32(reg(2), source, imm(1));
        if(checkSubnormal) {
          sub32(reg(3), reg(2), imm(1));
          cmp32(reg(3), imm(0x00ff'ffffu), set_ult);
          subnormal = jump(flag_ult);
        }
        if(checkQnan && !checkSnan) {
          cmp32(reg(2), imm(0xff80'0000u), set_uge);
          qnan = jump(flag_uge);
        } else if(!checkQnan && checkSnan) {
          xor32(reg(3), reg(2), imm(0x0040'0000u));
          cmp32(reg(3), imm(0xff80'0000u), set_ugt);
          snan = jump(flag_ugt);
        } else if(checkQnan && checkSnan) {
          cmp32(reg(2), imm(0xff00'0000u), set_ugt);
          snan = jump(flag_ugt);
        }
      }
    };

    // Run input checks for each input operand
    sljit_jump* qnanFs = nullptr;
    sljit_jump* snanFs = nullptr;
    sljit_jump* subnormalFs = nullptr;
    emitInputChecks(reg(0), qnanFs, snanFs, subnormalFs);

    sljit_jump* qnanFt = nullptr;
    sljit_jump* snanFt = nullptr;
    sljit_jump* subnormalFt = nullptr;
    if(inputCount == 2) emitInputChecks(reg(1), qnanFt, snanFt, subnormalFt);

    // Compute the trap mask and passthrough mask.
    // The trap mask is the bitmask of FPU flags that would trigger a VR4300 exception when signaled.
    // The passthrough mask is the bitmask of FPU flags that can be safely written back into
    // VR4300 FPU flags, as they won't trigger a VR4300 exception.
    u32 trapMask = 0;
    if(emitStateKey.fpuInvalidOperationEnabled()) trapMask |= fpuInvalidMask;
    if(emitStateKey.fpuDivisionByZeroEnabled())   trapMask |= fpuDiv0Mask;
    if(emitStateKey.fpuOverflowEnabled())         trapMask |= fpuOverflowMask;
    if(emitStateKey.fpuUnderflowEnabled())        trapMask |= fpuUnderflowMask;
    if(emitStateKey.fpuInexactEnabled())          trapMask |= fpuInexactMask;
    u32 passthrough = passthroughMask & fpuIeeeMask;
    if(!emitStateKey.fpuFlushSubnormals())        passthrough &= ~fpuUnderflowMask;
    u32 fallbackMask = trapMask | fpuDenormalMask | (fpuIeeeMask & ~passthrough);
    u32 stickyMask   = passthrough & ~trapMask;

    // EMIT: run the host FPU opcode
    sljit_jump* weird = nullptr;
#if defined(ARCHITECTURE_ARM64)
    // EMIT: backup FPCR, set it to the fast path value, and clear FPSR
    arm64ReadFpcr(reg(2));
    mov64(reg(3), imm(arm64FpuFastFpcr()));
    arm64WriteFpcr(reg(3));
    mov64(reg(3), imm(0));
    arm64WriteFpsr(reg(3));

    // EMIT: execute the host FPU opcode
    if(isDouble) {
      mov64(freg(0), reg(0));
      if(inputCount == 2) mov64(freg(1), reg(1));
    } else {
      mov32(freg(0), reg(0));
      if(inputCount == 2) mov32(freg(1), reg(1));
    }
    emitHostOpcode();
    if(isDouble) mov64(reg(1), freg(0));
    else mov32(reg(1), freg(0));

    // EMIT: read FPSR and restore FPCR
    arm64ReadFpsr(reg(3));
    arm64WriteFpcr(reg(2));

    // Special ARM64 compatibility fixup. In ARM64, when an underflow occurs,
    // the inexact flag is not set. Instead, in VR4300 and AMD64, it is set.
    if(passthrough & fpuUnderflowMask) {
      // EMIT: set the inexact flag if the underflow flag is set
      or32(reg(2), reg(3), imm(fpuInexactMask));
      test32(reg(3), imm(fpuUnderflowMask), set_z);
      cmov32(reg(3), reg(2), reg(3), flag_nz);
    }
#elif defined(ARCHITECTURE_AMD64)
    // EMIT: backup MXCSR, set it to the fast path value (which includes empty flags)
    amd64Stmxcsr(RecompilerFpuSaveMxcsrOffset);
    mov32(mem(sreg(0), RecompilerFpuFastMxcsrOffset), imm(amd64FpuFastMxcsr()));
    amd64Ldmxcsr(RecompilerFpuFastMxcsrOffset);

    // EMIT: execute the host FPU opcode
    if(isDouble) {
      mov64(freg(0), reg(0));
      if(inputCount == 2) mov64(freg(1), reg(1));
      emitHostOpcode();
      mov64(reg(1), freg(0));
    } else {
      mov32(freg(0), reg(0));
      if(inputCount == 2) mov32(freg(1), reg(1));
      emitHostOpcode();
      mov32(reg(1), freg(0));
    }

    // EMIT: read MXCSR and restore it
    amd64Stmxcsr(RecompilerFpuFastMxcsrOffset);
    amd64Ldmxcsr(RecompilerFpuSaveMxcsrOffset);
    mov32(reg(3), mem(sreg(0), RecompilerFpuFastMxcsrOffset));
#endif

    // EMIT: check if any FPU flags were signaled for which we must fallback
    // to the interpreter. If so, we jump to the slow path.
    test32(reg(3), imm(fallbackMask), set_z);
    weird = jump(flag_nz);

    // EMIT: update VR4300 FPU flags. Isolate the sticky bits (the safe passthrough ones),
    // and write them to the CAUSE bits. Then also update the FLAGS bits (with an OR,
    // because they are sticky across multiple instructions).
    and32(reg(0), reg(3), imm(stickyMask));
    mov32_u8(mem(sreg(0), FpuCsrCauseDataOffset), reg(0));
    or8(mem(sreg(0), FpuCsrFlagDataOffset), mem(sreg(0), FpuCsrFlagDataOffset), reg(0), reg(2));

    // EMIT: generate fixup to align the result to that expected by VR4300 in case of underflow
    if (passthrough & fpuUnderflowMask)
      emitFpuUnderflowFlushFixup(reg(3), reg(1), isDouble);

    // EMIT: write the result to the destination register.
    if(isDouble) {
      mov64(mem(sreg(2), fdWordOff), reg(1));
    } else {
      mov32(mem(sreg(2), fdWordOff), reg(1));
      mov32(mem(sreg(2), fdWordhOff), imm(0));
    }

    // Defer the slow paths.
    if(inputCount == 1) {
      deferSlowPath({qnanFs, snanFs, subnormalFs, weird}, instruction);
    } else {
      deferSlowPath({
        qnanFs, snanFs, subnormalFs,
        qnanFt, snanFt, subnormalFt,
        weird
      }, instruction);
    }

    // Account for the cycles taken by the slow path.
    emitDeferredCycles += cycles;
    return EmitExecuteResult::Linear;
  };

  auto emitFpuFixedRmToIntF32S32 = [&](u32 fixedRm) -> void {
    u32 m = fixedRm & 3u;
#if defined(ARCHITECTURE_ARM64)
    arm64FcvtS32FromF32(reg(1), freg(0), m);
#elif defined(ARCHITECTURE_AMD64)
    mov32(mem(sreg(0), RecompilerFpuFastMxcsrOffset), imm(amd64FpuOpMxcsr(m)));
    amd64Ldmxcsr(RecompilerFpuFastMxcsrOffset);
    amd64Cvtss2si32(reg(1), freg(0));
#endif
  };
  auto emitFpuFixedRmToIntF32S64 = [&](u32 fixedRm) -> void {
    u32 m = fixedRm & 3u;
#if defined(ARCHITECTURE_ARM64)
    arm64FcvtS64FromF32(reg(1), freg(0), m);
#elif defined(ARCHITECTURE_AMD64)
    mov32(mem(sreg(0), RecompilerFpuFastMxcsrOffset), imm(amd64FpuOpMxcsr(m)));
    amd64Ldmxcsr(RecompilerFpuFastMxcsrOffset);
    amd64Cvtss2si64(reg(1), freg(0));
#endif
  };
  auto emitFpuFixedRmToIntF64S32 = [&](u32 fixedRm) -> void {
    u32 m = fixedRm & 3u;
#if defined(ARCHITECTURE_ARM64)
    arm64FcvtS32FromF64(reg(1), freg(0), m);
#elif defined(ARCHITECTURE_AMD64)
    mov32(mem(sreg(0), RecompilerFpuFastMxcsrOffset), imm(amd64FpuOpMxcsr(m)));
    amd64Ldmxcsr(RecompilerFpuFastMxcsrOffset);
    amd64Cvtsd2si32(reg(1), freg(0));
#endif
  };
  auto emitFpuFixedRmToIntF64S64 = [&](u32 fixedRm) -> void {
    u32 m = fixedRm & 3u;
#if defined(ARCHITECTURE_ARM64)
    arm64FcvtS64FromF64(reg(1), freg(0), m);
#elif defined(ARCHITECTURE_AMD64)
    mov32(mem(sreg(0), RecompilerFpuFastMxcsrOffset), imm(amd64FpuOpMxcsr(m)));
    amd64Ldmxcsr(RecompilerFpuFastMxcsrOffset);
    amd64Cvtsd2si64(reg(1), freg(0));
#endif
  };

  auto emitFpuConvertOpcode = [&](
    auto&& callSlowPath,
    u32 fdn,
    u32 fsn,
    u32 sourceType,
    u32 destinationType,
    u32 cycles,
    auto&& emitHostOpcode
  ) -> EmitExecuteResult {
#if !defined(ARCHITECTURE_ARM64) && !defined(ARCHITECTURE_AMD64)
    return callSlowPath();
#endif
    if(emitSlowPathSection || !emitStateKey.coprocessor1Enabled()) {
      return callSlowPath();
    }

    // Conversion opcodes are fp<->integer or fp<->fp or int<->int. Create a few booleans to make
    // the code easier to read.
    const bool sourceIsInteger      = sourceType      == FpuConvertS32 || sourceType      == FpuConvertS64;
    const bool sourceIs64Bit        = sourceType      == FpuConvertF64 || sourceType      == FpuConvertS64;
    const bool destinationIsInteger = destinationType == FpuConvertS32 || destinationType == FpuConvertS64;
    const bool destinationIs64Bit   = destinationType == FpuConvertF64 || destinationType == FpuConvertS64;

    // Calculate input and output register offsets.
    s32 fsWordOff;
    if(sourceIs64Bit) {
      if(emitStateKey.floatingPointMode()) fsWordOff = (fsn - 16) * 8;
      else fsWordOff = ((fsn & ~1) - 16) * 8;
    } else {
      if(emitStateKey.floatingPointMode()) fsWordOff = (fsn - 16) * 8 + FpuR64S32Off;
      else fsWordOff = ((fsn & ~1) - 16) * 8 + FpuR64S32Off;
    }
    s32 fdWordOff = destinationIs64Bit ? (fdn - 16) * 8 : (fdn - 16) * 8 + FpuR64S32Off;
    s32 fdWordhOff = (fdn - 16) * 8 + FpuR64S32hOff;

    // EMIT: load the source operand
    if(sourceIs64Bit)
      mov64(reg(0), mem(sreg(2), fsWordOff));
    else
      mov32(reg(0), mem(sreg(2), fsWordOff));

    // On AMD64 and ARM64, all conversion opcodes raise an invlid operation flag on sNaN inputs. So there
    // is no need to make an explicit check.
    const bool checkSnan      = false;

    // On AMD64 and ARM64, the conversion opcodes for float<->double conversion do not raise any flag on qNaN inputs.
    // Thus, we do need to do an explicit check to fallback to the interpreter to mirror the actual behavior.
    const bool checkQnan      = !sourceIsInteger && !destinationIsInteger;

  #if defined(ARCHITECTURE_ARM64) 
    // On ARM64, input subnormals in conversion opcodes always raise the denormal flag in FPSR,
    // so there is not need to make an explicit check. The flag is not passhtorugh, so it will
    // force a fallback to the interpreter, as VR4300 instead always raises an unimplemented exception.
    const bool checkSubnormal = false;
  #elif defined(ARCHITECTURE_AMD64)
    // On AMD64, input subnormals in conversion opcodes are silently flushed to zero even without
    // explicitly requesting subnormal flushing (DAZ/FTZ). So we need to make an explicit check
    // to fallback to the interpreter, as VR4300 instead always raises an unimplemented exception.
    const bool checkSubnormal = true;
  #endif

    // EMIT: emit the input checks, required to fallback to the interpreter for inputs for which
    // the FPU behavior is different from VR4300.
    auto emitInputChecks = [&](reg source, sljit_jump*& qnan, sljit_jump*& snan, sljit_jump*& subnormal) -> void {
      if(sourceIsInteger) return;
      if(!checkSubnormal && !checkQnan && !checkSnan) return;
      if(sourceIs64Bit) {
        shl64(reg(2), source, imm(1));
        if(checkSubnormal) {
          sub64(reg(3), reg(2), imm(1));
          cmp64(reg(3), imm(0x001f'ffff'ffff'ffffull), set_ult);
          subnormal = jump(flag_ult);
        }
        if(checkQnan && !checkSnan) {
          cmp64(reg(2), imm(0xfff0'0000'0000'0000ull), set_uge);
          qnan = jump(flag_uge);
        } else if(!checkQnan && checkSnan) {
          xor64(reg(3), reg(2), imm(0x0008'0000'0000'0000ull));
          cmp64(reg(3), imm(0xfff0'0000'0000'0000ull), set_ugt);
          snan = jump(flag_ugt);
        } else if(checkQnan && checkSnan) {
          cmp64(reg(2), imm(0xffe0'0000'0000'0000ull), set_ugt);
          snan = jump(flag_ugt);
        }
      } else {
        shl32(reg(2), source, imm(1));
        if(checkSubnormal) {
          sub32(reg(3), reg(2), imm(1));
          cmp32(reg(3), imm(0x00ff'ffffu), set_ult);
          subnormal = jump(flag_ult);
        }
        if(checkQnan && !checkSnan) {
          cmp32(reg(2), imm(0xff80'0000u), set_uge);
          qnan = jump(flag_uge);
        } else if(!checkQnan && checkSnan) {
          xor32(reg(3), reg(2), imm(0x0040'0000u));
          cmp32(reg(3), imm(0xff80'0000u), set_ugt);
          snan = jump(flag_ugt);
        } else if(checkQnan && checkSnan) {
          cmp32(reg(2), imm(0xff00'0000u), set_ugt);
          snan = jump(flag_ugt);
        }
      }
    };

    sljit_jump* qnanFs = nullptr;
    sljit_jump* snanFs = nullptr;
    sljit_jump* subnormalFs = nullptr;
    emitInputChecks(reg(0), qnanFs, snanFs, subnormalFs);

    sljit_jump* outOfRangeHigh = nullptr;
    sljit_jump* outOfRangeLow = nullptr;

    // Enforce VR4300-specific limit of |src| <= 2^55 for conversion from integers
    // For this, we fallback to the interpreter to raise an unimplemented exception.
    if(sourceType == FpuConvertS64) {
      assert(destinationType == FpuConvertF32 || destinationType == FpuConvertF64);
      cmp64(reg(0), imm(0x0080'0000'0000'0000ull), set_sge);
      outOfRangeHigh = jump(flag_sge);
      cmp64(reg(0), imm(-0x0080'0000'0000'0000ll), set_slt);
      outOfRangeLow = jump(flag_slt);
    }

    // EMIT: enforce VR4300-specific limit of |src| <= 2^53 for conversion to integers.
    // For this, we fallback to the interpreter to raise an unimplemented exception.
    if(sourceType == FpuConvertF32 && destinationType == FpuConvertS64) {
      shl32(reg(2), reg(0), imm(1));
      cmp32(reg(2), imm(0xb400'0000u), set_uge);
      outOfRangeHigh = jump(flag_uge);
    }
    if(sourceType == FpuConvertF64 && destinationType == FpuConvertS64) {
      shl64(reg(2), reg(0), imm(1));
      cmp64(reg(2), imm(0x8680'0000'0000'0000ull), set_uge);
      outOfRangeHigh = jump(flag_uge);
    }

    // Calculate the fallback mask and passthrough mask.
    // passhtrough mask: bits that when flagged by the host FPU, can be safely
    //   passthrough to the control register of the emulated FPU.
    // fallback mask: bits that when flagged by the host FPU, must force a fallback
    //   to the interpreter for proper behavior.
    u32 trapMask = 0;
    if(emitStateKey.fpuInvalidOperationEnabled()) trapMask |= fpuInvalidMask;
    if(emitStateKey.fpuDivisionByZeroEnabled())   trapMask |= fpuDiv0Mask;
    if(emitStateKey.fpuOverflowEnabled())         trapMask |= fpuOverflowMask;
    if(emitStateKey.fpuUnderflowEnabled())        trapMask |= fpuUnderflowMask;
    if(emitStateKey.fpuInexactEnabled())          trapMask |= fpuInexactMask;
    u32 passthrough = fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask;
    if(!sourceIsInteger && !emitStateKey.fpuFlushSubnormals()) passthrough &= ~fpuUnderflowMask;
    // Fallback is required for:
    // - Any bit where the VR4300 is configured to trap (trapMask)
    // - Invalid operation. This is raised on sNaN for instance.
    // - Denormal. On VR4300, all denormals cause not implemented exception.
    // - Any bit that cannot be safely passthrough.
    u32 fallbackMask = trapMask | fpuInvalidMask | fpuDenormalMask | (fpuIeeeMask & ~passthrough);
    u32 stickyMask   = passthrough & ~trapMask;

    sljit_jump* weird = nullptr;
#if defined(ARCHITECTURE_ARM64)
    arm64ReadFpcr(reg(2));
    mov64(reg(3), imm(arm64FpuFastFpcr()));
    arm64WriteFpcr(reg(3));
    mov64(reg(3), imm(0));
    arm64WriteFpsr(reg(3));

    if(!sourceIsInteger) {
      if(sourceIs64Bit) mov64(freg(0), reg(0));
      else mov32(freg(0), reg(0));
    }
    emitHostOpcode();
    if(!destinationIsInteger) {
      if(destinationIs64Bit) mov64(reg(1), freg(0));
      else mov32(reg(1), freg(0));
    }
    arm64ReadFpsr(reg(3));
    arm64WriteFpcr(reg(2));
    if(passthrough & fpuUnderflowMask) {
      or32(reg(2), reg(3), imm(fpuInexactMask));
      test32(reg(3), imm(fpuUnderflowMask), set_z);
      cmov32(reg(3), reg(2), reg(3), flag_nz);
    }
#elif defined(ARCHITECTURE_AMD64)
    amd64Stmxcsr(RecompilerFpuSaveMxcsrOffset);
    mov32(mem(sreg(0), RecompilerFpuFastMxcsrOffset), imm(amd64FpuFastMxcsr()));
    amd64Ldmxcsr(RecompilerFpuFastMxcsrOffset);

    if(!sourceIsInteger) {
      if(sourceIs64Bit) mov64(freg(0), reg(0));
      else mov32(freg(0), reg(0));
    }
    emitHostOpcode();
    if(!destinationIsInteger) {
      if(destinationIs64Bit) mov64(reg(1), freg(0));
      else mov32(reg(1), freg(0));
    }
    amd64Stmxcsr(RecompilerFpuFastMxcsrOffset);
    amd64Ldmxcsr(RecompilerFpuSaveMxcsrOffset);
    mov32(reg(3), mem(sreg(0), RecompilerFpuFastMxcsrOffset));
#endif

    // EMIT: if any flag in the fallback mask is set, we need to fallback to the interpreter.
    test32(reg(3), imm(fallbackMask), set_z);
    weird = jump(flag_nz);

    // EMIT: store flags into the emulated Cause and Flag bits
    and32(reg(0), reg(3), imm(stickyMask));
    mov32_u8(mem(sreg(0), FpuCsrCauseDataOffset), reg(0));
    or8(mem(sreg(0), FpuCsrFlagDataOffset), mem(sreg(0), FpuCsrFlagDataOffset), reg(0), reg(2));

    // EMIT: generate fixup to align the result to that expected by VR4300 in case of underflow
    if(!sourceIsInteger && !destinationIsInteger && (passthrough & fpuUnderflowMask))
      emitFpuUnderflowFlushFixup(reg(3), reg(1), destinationType == FpuConvertF64);

    // EMIT: store the result
    if(destinationIs64Bit) {
      mov64(mem(sreg(2), fdWordOff), reg(1));
    } else {
      mov32(mem(sreg(2), fdWordOff), reg(1));
      mov32(mem(sreg(2), fdWordhOff), imm(0));
    }

    deferSlowPath({
      qnanFs, snanFs, subnormalFs,
      outOfRangeHigh, outOfRangeLow,
      weird
    }, instruction);
    emitDeferredCycles += cycles;
    return EmitExecuteResult::Linear;
  };

  auto emitFpuCompareOpcode = [&](
    auto&& callSlowPath,
    u32 fsn,
    u32 ftn,
    u32 width,
    u32 cycles,
    u32 inputChecks,
    u32 nanResult,
    u32 nanInvalidPolicy,
    u32 compareKind
  ) -> EmitExecuteResult {
#if !defined(ARCHITECTURE_ARM64) && !defined(ARCHITECTURE_AMD64)
    return callSlowPath();
#endif
    if(emitSlowPathSection || !emitStateKey.coprocessor1Enabled()) {
      return callSlowPath();
    }

    assert(width == FpuWidth32 || width == FpuWidth64);
    assert(inputChecks & FpuCheckQnan);
    assert(inputChecks & FpuCheckSnan);
    bool isDouble = width == FpuWidth64;
    s32 fsWordOff;
    s32 ftWordOff;
    if(isDouble) {
      if(emitStateKey.floatingPointMode()) fsWordOff = (fsn - 16) * 8;
      else fsWordOff = ((fsn & ~1) - 16) * 8;
      ftWordOff = (ftn - 16) * 8;
    } else {
      if(emitStateKey.floatingPointMode()) {
        fsWordOff = (fsn - 16) * 8 + FpuR64S32Off;
      } else {
        fsWordOff = ((fsn & ~1) - 16) * 8 + FpuR64S32Off;
      }
      ftWordOff = (ftn - 16) * 8 + FpuR64S32Off;
    }

    if(isDouble) {
      mov64(reg(0), mem(sreg(2), fsWordOff));
      mov64(reg(1), mem(sreg(2), ftWordOff));
    } else {
      mov32(reg(0), mem(sreg(2), fsWordOff));
      mov32(reg(1), mem(sreg(2), ftWordOff));
    }
    mov32_u8(mem(sreg(0), FpuCsrCauseDataOffset), imm(0));

    sljit_jump* nanFs = nullptr;
    sljit_jump* nanFt = nullptr;
    sljit_jump* qnanFs = nullptr;
    sljit_jump* qnanFt = nullptr;
    auto emitQnanCheck = [&](reg source, sljit_jump*& qnan) -> void {
      if(isDouble) {
        shl64(reg(2), source, imm(1));
        cmp64(reg(2), imm(0xfff0'0000'0000'0000ull), set_uge);
        qnan = jump(flag_uge);
      } else {
        shl32(reg(2), source, imm(1));
        cmp32(reg(2), imm(0xff80'0000u), set_uge);
        qnan = jump(flag_uge);
      }
    };
    auto emitNanCheck = [&](reg source, sljit_jump*& nan) -> void {
      if(isDouble) {
        shl64(reg(2), source, imm(1));
        cmp64(reg(2), imm(0xffe0'0000'0000'0000ull), set_ugt);
        nan = jump(flag_ugt);
      } else {
        shl32(reg(2), source, imm(1));
        cmp32(reg(2), imm(0xff00'0000u), set_ugt);
        nan = jump(flag_ugt);
      }
    };
    emitQnanCheck(reg(0), qnanFs);
    emitQnanCheck(reg(1), qnanFt);
    emitNanCheck(reg(0), nanFs);
    emitNanCheck(reg(1), nanFt);

    switch(compareKind) {
    case FpuCompareFalse:
      mov32(reg(2), imm(0));
      break;
    case FpuCompareEq: {
      if(isDouble) {
        mov64(freg(0), reg(0));
        mov64(freg(1), reg(1));
        fcmp64(freg(0), freg(1));
        mov32(reg(2), imm(0));
        mov32(reg(3), imm(1));
        cmov32(reg(2), reg(3), reg(2), flag_feq);
      } else {
        mov32(freg(0), reg(0));
        mov32(freg(1), reg(1));
        fcmp32(freg(0), freg(1));
        mov32(reg(2), imm(0));
        mov32(reg(3), imm(1));
        cmov32(reg(2), reg(3), reg(2), flag_feq);
      }
      break;
    }
    case FpuCompareLt: {
      if(isDouble) {
        mov64(freg(0), reg(0));
        mov64(freg(1), reg(1));
        fcmp64(freg(0), freg(1));
        mov32(reg(2), imm(0));
        mov32(reg(3), imm(1));
        cmov32(reg(2), reg(3), reg(2), flag_flt);
      } else {
        mov32(freg(0), reg(0));
        mov32(freg(1), reg(1));
        fcmp32(freg(0), freg(1));
        mov32(reg(2), imm(0));
        mov32(reg(3), imm(1));
        cmov32(reg(2), reg(3), reg(2), flag_flt);
      }
      break;
    }
    case FpuCompareLe: {
      if(isDouble) {
        mov64(freg(0), reg(0));
        mov64(freg(1), reg(1));
        fcmp64(freg(0), freg(1));
        mov32(reg(2), imm(0));
        mov32(reg(3), imm(1));
        cmov32(reg(2), reg(3), reg(2), flag_fle);
      } else {
        mov32(freg(0), reg(0));
        mov32(freg(1), reg(1));
        fcmp32(freg(0), freg(1));
        mov32(reg(2), imm(0));
        mov32(reg(3), imm(1));
        cmov32(reg(2), reg(3), reg(2), flag_fle);
      }
      break;
    }
    default:
      unreachable;
    }
    mov32_u8(FpuCsrCompare, reg(2));
    auto done = jump();

    if(nanInvalidPolicy == FpuCompareUnordered) {
      if(!emitStateKey.fpuInvalidOperationEnabled()) {
        setLabel(qnanFs); qnanFs = nullptr;
        setLabel(qnanFt); qnanFt = nullptr;
        mov32_u8(mem(sreg(0), FpuCsrCauseDataOffset), imm(fpuInvalidMask));
        or8(mem(sreg(0), FpuCsrFlagDataOffset), mem(sreg(0), FpuCsrFlagDataOffset), imm(fpuInvalidMask), reg(2));
        setLabel(nanFs); nanFs = nullptr;
        setLabel(nanFt); nanFt = nullptr;
        mov32_u8(FpuCsrCompare, imm(nanResult & 1));
      }
    }
    if(nanInvalidPolicy == FpuCompareOrdered) {
      if(!emitStateKey.fpuInvalidOperationEnabled()) {
        setLabel(qnanFs); qnanFs = nullptr;
        setLabel(qnanFt); qnanFt = nullptr;
        setLabel(nanFs); nanFs = nullptr;
        setLabel(nanFt); nanFt = nullptr;
        mov32_u8(mem(sreg(0), FpuCsrCauseDataOffset), imm(fpuInvalidMask));
        or8(mem(sreg(0), FpuCsrFlagDataOffset), mem(sreg(0), FpuCsrFlagDataOffset), imm(fpuInvalidMask), reg(2));
        mov32_u8(FpuCsrCompare, imm(nanResult & 1));
      }
    }
    setLabel(done);
    deferSlowPath({ qnanFs, qnanFt, nanFs, nanFt }, instruction);
    emitDeferredCycles += cycles;
    return EmitExecuteResult::Linear;
  };
#endif

  switch(instruction >> 21 & 0x1f) {

  //MFC1 Rt,Fs
  case 0x00: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::MFC1, mem(Rt), imm(Fsn));
      return EmitExecuteResult::MayFault;
    }
    if(Rtn == 0) return EmitExecuteResult::Linear;
    s32 fsn = instruction >> 11 & 31;
    s32 fpuWordOff;
    if(emitStateKey.floatingPointMode()) {
      fpuWordOff = (fsn - 16) * 8 + FpuR64S32Off;
      mov32(reg(0), mem(sreg(2), fpuWordOff));
    } else {
      s32 paired = fsn & ~1;
      fpuWordOff = (paired - 16) * 8;
      if(fsn & 1) fpuWordOff += FpuR64S32hOff;
      else fpuWordOff += FpuR64S32Off;
      mov32(reg(0), mem(sreg(2), fpuWordOff));
    }
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rt), reg(0));
    return EmitExecuteResult::Linear;
  }

  //DMFC1 Rt,Fs
  case 0x01: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::DMFC1, mem(Rt), imm(Fsn));
      return EmitExecuteResult::MayFault;
    }
    if(Rtn == 0) return EmitExecuteResult::Linear;
    s32 fsn = instruction >> 11 & 31;
    s32 fpu64Off;
    if(emitStateKey.floatingPointMode()) fpu64Off = (fsn - 16) * 8;
    else fpu64Off = ((fsn & ~1) - 16) * 8;
    mov64(reg(0), mem(sreg(2), fpu64Off));
    mov64(mem(Rt), reg(0));
    return EmitExecuteResult::Linear;
  }

  //CFC1 Rt,Rd
  case 0x02: {
    setupCallf();
    callf(&CPU::CFC1, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
  }

  //DCFC1 Rt,Rd
  case 0x03: {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return EmitExecuteResult::MayFault;
  }

  //MTC1 Rt,Fs
  case 0x04: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::MTC1, mem(Rt), imm(Fsn));
      return EmitExecuteResult::MayFault;
    }
    mov32(reg(0), mem(Rt32));
    s32 fsn = instruction >> 11 & 31;
    s32 fpuWordOff;
    if(emitStateKey.floatingPointMode()) {
      fpuWordOff = (fsn - 16) * 8 + FpuR64S32Off;
      mov32(mem(sreg(2), fpuWordOff), reg(0));
    } else {
      s32 paired = fsn & ~1;
      fpuWordOff = (paired - 16) * 8;
      if(fsn & 1) fpuWordOff += FpuR64S32hOff;
      else fpuWordOff += FpuR64S32Off;
      mov32(mem(sreg(2), fpuWordOff), reg(0));
    }
    return EmitExecuteResult::Linear;
  }

  //DMTC1 Rt,Fs
  case 0x05: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::DMTC1, mem(Rt), imm(Fsn));
      return EmitExecuteResult::MayFault;
    }
    s32 fsn = instruction >> 11 & 31;
    s32 fpu64Off;
    if(emitStateKey.floatingPointMode()) fpu64Off = (fsn - 16) * 8;
    else fpu64Off = ((fsn & ~1) - 16) * 8;
    mov64(reg(0), mem(Rt));
    mov64(mem(sreg(2), fpu64Off), reg(0));
    return EmitExecuteResult::Linear;
  }

  //CTC1 Rt,Rd
  case 0x06: {
    setupCallf();
    callf(&CPU::CTC1, mem(Rt), imm(Rdn));
    return EmitExecuteResult::MayFault;
  }

  //DCTC1 Rt,Rd
  case 0x07: {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return EmitExecuteResult::MayFault;
  }

  //BC1 offset
  case 0x08: {
    auto emitBranchTarget = [&](s16 offset) -> void {
      if(pcMode == EmitPcMode::Runtime) {
        add64(reg(0), PipelineReg(pc), imm(s32(offset) * 4));
        mov64(PipelineReg(nextpc), reg(0));
        return;
      }
      mov64(PipelineReg(nextpc), imm(emitVaddr + 4 + s64(s32(offset) * 4)));
    };
    auto emitLikelyNotTaken = [&] {
      if(pcMode == EmitPcMode::Runtime) {
        add64(reg(0), PipelineReg(pc), imm(4));
        mov64(PipelineReg(pc), reg(0));
        add64(PipelineReg(nextpc), reg(0), imm(4));
      } else {
        mov64(PipelineReg(pc), imm(emitVaddr + 8));
        mov64(PipelineReg(nextpc), imm(emitVaddr + 12));
      }
      or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    };
    bool value = instruction >> 16 & 1;
    bool likely = instruction >> 17 & 1;
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::BC1, imm(value), imm(likely), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    movzeron(FpuCsrCauseOffset, sizeof(CPU::FPU::ControlStatus::Cause));
    mov32(reg(0), FpuCsrCompare);
    and32(reg(0), reg(0), imm(1));
    cmp32(reg(0), imm(value), set_z);
    auto taken = jump(flag_z);
    if(likely) {
      emitLikelyNotTaken();
    } else {
      mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    }
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //INVALID
  case range7(0x09, 0x0f): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  }

  if((instruction >> 21 & 31) == 16)
  switch(instruction & 0x3f) {

  //FADD.S Fd,Fs,Ft
  case 0x00: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FADD_S, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth32, (5 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fadd32(freg(0), freg(0), freg(1));
      }
    );
  }

  //FSUB.S Fd,Fs,Ft
  case 0x01: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FSUB_S, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth32, (5 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fsub32(freg(0), freg(0), freg(1));
      }
    );
  }

  //FMUL.S Fd,Fs,Ft
  case 0x02: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FMUL_S, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth32, (5 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fmul32(freg(0), freg(0), freg(1));
      }
    );
  }

  //FDIV.S Fd,Fs,Ft
  case 0x03: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FDIV_S, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth32, (29 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fdiv32(freg(0), freg(0), freg(1));
      }
    );
  }

  //FSQRT.S Fd,Fs
  case 0x04: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FSQRT_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn}, FpuWidth32, (29 - 1) * 2, FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fsqrt32_f0();
      }
    );
  }

  //FABS.S Fd,Fs
  case 0x05: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FABS_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn}, FpuWidth32, 0, FpuCheckQnan | FpuCheckSnan | FpuCheckSubnormal,
      fpuInvalidMask | fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fabs32(freg(0), freg(0));
      }
    );
  }

  //FMOV.S Fd,Fs
  case 0x06: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::FMOV_S, imm(Fdn), imm(Fsn));
      return EmitExecuteResult::MayFault;
    }
    s32 fsn = Fsn, fdn = Fdn;
    s32 fsWordOff = emitStateKey.floatingPointMode() ? (fsn - 16) * 8 : ((fsn & ~1) - 16) * 8;
    s32 fdWordOff = (fdn - 16) * 8;
    mov64(reg(0), mem(sreg(2), fsWordOff));
    mov64(mem(sreg(2), fdWordOff), reg(0));
    return EmitExecuteResult::Linear;
  }

  //FNEG.S Fd,Fs
  case 0x07: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FNEG_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn}, FpuWidth32, 0, FpuCheckQnan | FpuCheckSnan | FpuCheckSubnormal,
      fpuInvalidMask | fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fneg32(freg(0), freg(0));
      }
    );
  }

  //FROUND.L.S Fd,Fs
  case 0x08: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FROUND_L_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S64(0u);
      }
    );
  }

  //FTRUNC.L.S Fd,Fs
  case 0x09: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FTRUNC_L_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S64(1u);
      }
    );
  }

  //FCEIL.L.S Fd,Fs
  case 0x0a: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCEIL_L_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S64(2u);
      }
    );
  }

  //FFLOOR.L.S Fd,Fs
  case 0x0b: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FFLOOR_L_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::Linear;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S64(3u);
      }
    );
  }

  //FROUND.W.S Fd,Fs
  case 0x0c: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FROUND_W_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S32(0u);
      }
    );
  }

  //FTRUNC.W.S Fd,Fs
  case 0x0d: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FTRUNC_W_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S32(1u);
      }
    );
  }

  //FCEIL.W.S Fd,Fs
  case 0x0e: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCEIL_W_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S32(2u);
      }
    );
  }

  //FFLOOR.W.S Fd,Fs
  case 0x0f: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FFLOOR_W_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF32S32(3u);
      }
    );
  }

  //FCVT.S.S Fd,Fs
  case 0x20: {
    setupCallf();
    callf(&CPU::FCVT_S_S, imm(Fdn), imm(Fsn));
    return EmitExecuteResult::MayFault;
  }

  //FCVT.D.S Fd,Fs
  case 0x21: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_D_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertF64, 0,
      [&]() -> void {
        conv_f64_from_f32(freg(0), freg(0));
      }
    );
  }

  //FCVT.W.S Fd,Fs
  case 0x24: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_W_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
#if defined(ARCHITECTURE_ARM64)
        arm64FcvtS32FromF32(reg(1), freg(0), (u32)emitStateKey.fpuRoundMode() & 3u);
#elif defined(ARCHITECTURE_AMD64)
        amd64Cvtss2si32(reg(1), freg(0));
#endif
      }
    );
  }

  //FCVT.L.S Fd,Fs
  case 0x25: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_L_S, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF32, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
#if defined(ARCHITECTURE_ARM64)
        arm64FcvtS64FromF32(reg(1), freg(0), (u32)emitStateKey.fpuRoundMode() & 3u);
#elif defined(ARCHITECTURE_AMD64)
        amd64Cvtss2si64(reg(1), freg(0));
#endif
      }
    );
  }

  //FC.F.S Fs,Ft
  case 0x30: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_F_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareFalse
    );
  }

  //FC.UN.S Fs,Ft
  case 0x31: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_UN_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareFalse
    );
  }

  //FC.EQ.S Fs,Ft
  case 0x32: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_EQ_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareEq
    );
  }

  //FC.UEQ.S Fs,Ft
  case 0x33: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_UEQ_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareEq
    );
  }

  //FC.OLT.S Fs,Ft
  case 0x34: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_OLT_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareLt
    );
  }

  //FC.ULT.S Fs,Ft
  case 0x35: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_ULT_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareLt
    );
  }

  //FC.OLE.S Fs,Ft
  case 0x36: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_OLE_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareLe
    );
  }

  //FC.ULE.S Fs,Ft
  case 0x37: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_ULE_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareLe
    );
  }

  //FC.SF.S Fs,Ft
  case 0x38: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_SF_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareFalse
    );
  }

  //FC.NGLE.S Fs,Ft
  case 0x39: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGLE_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareFalse
    );
  }

  //FC.SEQ.S Fs,Ft
  case 0x3a: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_SEQ_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareEq
    );
  }

  //FC.NGL.S Fs,Ft
  case 0x3b: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGL_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareEq
    );
  }

  //FC.LT.S Fs,Ft
  case 0x3c: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_LT_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::Linear;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareLt
    );
  }

  //FC.NGE.S Fs,Ft
  case 0x3d: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGE_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareLt
    );
  }

  //FC.LE.S Fs,Ft
  case 0x3e: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_LE_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareLe
    );
  }

  //FC.NGT.S Fs,Ft
  case 0x3f: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGT_S, imm(Fsn), imm(Ftn));
        return EmitExecuteResult::Linear;
      },
      Fsn, Ftn, FpuWidth32, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareLe
    );
  }
  }

  if((instruction >> 21 & 31) == 17)
  switch(instruction & 0x3f) {

//FADD.D Fd,Fs,Ft
  case 0x00: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FADD_D, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth64, (3 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fadd64(freg(0), freg(0), freg(1));
      }
    );
  }

  //FSUB.D Fd,Fs,Ft
  case 0x01: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FSUB_D, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth64, (3 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fsub64(freg(0), freg(0), freg(1));
      }
    );
  }

  //FMUL.D Fd,Fs,Ft
  case 0x02: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FMUL_D, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth64, (8 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fmul64(freg(0), freg(0), freg(1));
      }
    );
  }

  //FDIV.D Fd,Fs,Ft
  case 0x03: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FDIV_D, imm(Fdn), imm(Fsn), imm(Ftn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn, Ftn}, FpuWidth64, (58 - 1) * 2,
      FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fdiv64(freg(0), freg(0), freg(1));
      }
    );
  }

  //FSQRT.D Fd,Fs
  case 0x04: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FSQRT_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn}, FpuWidth64, (58 - 1) * 2, FpuCheckQnan,
      fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fsqrt64_f0();
      }
    );
  }

  //FABS.D Fd,Fs
  case 0x05: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FABS_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn}, FpuWidth64, 0, FpuCheckQnan | FpuCheckSnan | FpuCheckSubnormal,
      fpuInvalidMask | fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fabs64(freg(0), freg(0));
      }
    );
  }

  //FMOV.D Fd,Fs
  case 0x06: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::FMOV_D, imm(Fdn), imm(Fsn));
      return EmitExecuteResult::MayFault;
    }
    s32 fsn = Fsn, fdn = Fdn;
    s32 fsWordOff = emitStateKey.floatingPointMode() ? (fsn - 16) * 8 : ((fsn & ~1) - 16) * 8;
    s32 fdWordOff = (fdn - 16) * 8;
    mov64(reg(0), mem(sreg(2), fsWordOff));
    mov64(mem(sreg(2), fdWordOff), reg(0));
    return EmitExecuteResult::Linear;
  }

  //FNEG.D Fd,Fs
  case 0x07: {
    return emitFpuOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FNEG_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, {Fsn}, FpuWidth64, 0, FpuCheckQnan | FpuCheckSnan | FpuCheckSubnormal,
      fpuInvalidMask | fpuDiv0Mask | fpuOverflowMask | fpuUnderflowMask | fpuInexactMask,
      [&]() -> void {
        fneg64(freg(0), freg(0));
      }
    );
  }

  //FROUND.L.D Fd,Fs
  case 0x08: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FROUND_L_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S64(0u);
      }
    );
  }

  //FTRUNC.L.D Fd,Fs
  case 0x09: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FTRUNC_L_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S64(1u);
      }
    );
  }

  //FCEIL.L.D Fd,Fs
  case 0x0a: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCEIL_L_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S64(2u);
      }
    );
  }

  //FFLOOR.L.D Fd,Fs
  case 0x0b: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FFLOOR_L_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S64(3u);
      }
    );
  }

  //FROUND.W.D Fd,Fs
  case 0x0c: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FROUND_W_D, imm(Fdn), imm(Fsn));
      return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S32(0u);
      }
    );
  }

  //FTRUNC.W.D Fd,Fs
  case 0x0d: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FTRUNC_W_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S32(1u);
      }
    );
  }

  //FCEIL.W.D Fd,Fs
  case 0x0e: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCEIL_W_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S32(2u);
      }
    );
  }

  //FFLOOR.W.D Fd,Fs
  case 0x0f: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FFLOOR_W_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
        emitFpuFixedRmToIntF64S32(3u);
      }
    );
  }

  //FCVT.S.D Fd,Fs
  case 0x20: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_S_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertF32, (2 - 1) * 2,
      [&]() -> void {
        conv_f32_from_f64(freg(0), freg(0));
      }
    );
  }

  //FCVT.D.D Fd,Fs
  case 0x21: {
    setupCallf();
    callf(&CPU::FCVT_D_D, imm(Fdn), imm(Fsn));
    return EmitExecuteResult::MayFault;
  }

  //FCVT.W.D Fd,Fs
  case 0x24: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_W_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS32, (5 - 1) * 2,
      [&]() -> void {
#if defined(ARCHITECTURE_ARM64)
        arm64FcvtS32FromF64(reg(1), freg(0), (u32)emitStateKey.fpuRoundMode() & 3u);
#elif defined(ARCHITECTURE_AMD64)
        amd64Cvtsd2si32(reg(1), freg(0));
#endif
      }
    );
  }

  //FCVT.L.D Fd,Fs
  case 0x25: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_L_D, imm(Fdn), imm(Fsn));
        return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertF64, FpuConvertS64, (5 - 1) * 2,
      [&]() -> void {
#if defined(ARCHITECTURE_ARM64)
        arm64FcvtS64FromF64(reg(1), freg(0), (u32)emitStateKey.fpuRoundMode() & 3u);
#elif defined(ARCHITECTURE_AMD64)
        amd64Cvtsd2si64(reg(1), freg(0));
#endif
      }
    );
  }

  //FC.F.D Fs,Ft
  case 0x30: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_F_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareFalse
    );
  }

  //FC.UN.D Fs,Ft
  case 0x31: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_UN_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareFalse
    );
  }

  //FC.EQ.D Fs,Ft
  case 0x32: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_EQ_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareEq
    );
  }

  //FC.UEQ.D Fs,Ft
  case 0x33: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_UEQ_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareEq
    );
  }

  //FC.OLT.D Fs,Ft
  case 0x34: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_OLT_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareLt
    );
  }

  //FC.ULT.D Fs,Ft
  case 0x35: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_ULT_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareLt
    );
  }

  //FC.OLE.D Fs,Ft
  case 0x36: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_OLE_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareUnordered, FpuCompareLe
    );
  }

  //FC.ULE.D Fs,Ft
  case 0x37: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_ULE_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareUnordered, FpuCompareLe
    );
  }

  //FC.SF.D Fs,Ft
  case 0x38: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_SF_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareFalse
    );
  }

  //FC.NGLE.D Fs,Ft
  case 0x39: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGLE_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareFalse
    );
  }

  //FC.SEQ.D Fs,Ft
  case 0x3a: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_SEQ_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareEq
    );
  }

  //FC.NGL.D Fs,Ft
  case 0x3b: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGL_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareEq
    );
  }

  //FC.LT.D Fs,Ft
  case 0x3c: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_LT_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareLt
    );
  }

  //FC.NGE.D Fs,Ft
  case 0x3d: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGE_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareLt
    );
  }

  //FC.LE.D Fs,Ft
  case 0x3e: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_LE_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 0,
      FpuCompareOrdered, FpuCompareLe
    );
  }

  //FC.NGT.D Fs,Ft
  case 0x3f: {
    return emitFpuCompareOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FC_NGT_D, imm(Fsn), imm(Ftn));
      return EmitExecuteResult::MayFault;
      },
      Fsn, Ftn, FpuWidth64, (3 - 1) * 2, FpuCheckQnan | FpuCheckSnan, 1,
      FpuCompareOrdered, FpuCompareLe
    );
  }

  }

  if((instruction >> 21 & 31) == 20)
  switch(instruction & 0x3f) {
  case range8(0x08, 0x0f): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return EmitExecuteResult::MayFault;
  }

  case range2(0x24, 0x25): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return EmitExecuteResult::MayFault;
  }

  //FCVT.S.W Fd,Fs
  case 0x20: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_S_W, imm(Fdn), imm(Fsn));
      return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertS32, FpuConvertF32, (5 - 1) * 2,
      [&]() -> void {
        conv_f32_from_s32(freg(0), reg(0));
      }
    );
  }

  //FCVT.D.W Fd,Fs
  case 0x21: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_D_W, imm(Fdn), imm(Fsn));
      return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertS32, FpuConvertF64, (5 - 1) * 2,
      [&]() -> void {
        conv_f64_from_s32(freg(0), reg(0));
      }
    );
  }

  }

  if((instruction >> 21 & 31) == 21)
  switch(instruction & 0x3f) {
  case range8(0x08, 0x0f): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return EmitExecuteResult::MayFault;
  }
  case range2(0x24, 0x25): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return EmitExecuteResult::MayFault;
  }

  //FCVT.S.L
  case 0x20: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_S_L, imm(Fdn), imm(Fsn));
      return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertS64, FpuConvertF32, (5 - 1) * 2,
      [&]() -> void {
        conv_f32_from_sw(freg(0), reg(0));
      }
    );
  }

  //FCVT.D.L
  case 0x21: {
    return emitFpuConvertOpcode(
      [&]() -> EmitExecuteResult {
        setupCallf();
        callf(&CPU::FCVT_D_L, imm(Fdn), imm(Fsn));
      return EmitExecuteResult::MayFault;
      },
      Fdn, Fsn, FpuConvertS64, FpuConvertF64, (5 - 1) * 2,
      [&]() -> void {
        conv_f64_from_sw(freg(0), reg(0));
      }
    );
  }

  }

  return EmitExecuteResult::Linear;
}
