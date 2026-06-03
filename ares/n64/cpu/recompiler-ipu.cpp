auto CPU::Recompiler::emitZeroClear(u32 n) -> void {
  if(n == 0) mov64(mem(IpuReg(r[0])), imm(0));
}

auto CPU::Recompiler::emitCpuStep(u32 clocks) -> void {
  add64(CpuClockMem, CpuClockMem, imm(clocks));
}

auto CPU::Recompiler::flushDeferredCycles() -> void {
  if(!emitDeferredCycles) return;
  emitCpuStep(emitDeferredCycles);
  emitDeferredCycles = 0;
}

auto CPU::Recompiler::setupPipeline() -> void {
  if(emitPipelineSetupDone) return;
  mov32(PipelineReg(nstate), imm(0));
  if(emitPcMode == EmitPcMode::Runtime) {
    mov64(reg(0), PipelineReg(nextpc));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
  } else {
    mov64(PipelineReg(pc), imm(emitVaddr + 4));
    mov64(PipelineReg(nextpc), imm(emitVaddr + 8));
  }
  emitPipelineSetupDone = true;
}

auto CPU::Recompiler::setupCallf() -> void {
  if(emitCallfSetupDone) return;
  setupPipeline();
  flushDeferredCycles();
  mov64(mem(IpuReg(pc)), imm(emitVaddr));
  emitCallfSetupDone = true;
  emitCallfEmitted = true;
}

auto CPU::Recompiler::deferSlowPath(sljit_jump* enter, u32 instruction) -> void {
  deferSlowPath({enter}, instruction);
}

auto CPU::Recompiler::deferSlowPath(std::initializer_list<sljit_jump*> enters, u32 instruction) -> void {
  auto& slow = slowPaths.emplace_back();
  for(auto enter : enters) {
    if(enter) slow.enters.push_back(enter);
  }
  slow.instruction = instruction;
  slow.icacheMiss = false;
  slow.runtimePc = emitPcMode == EmitPcMode::Runtime;
  slow.vaddr = emitVaddr;
  slow.deferredCycles = emitDeferredCycles;
}

auto CPU::Recompiler::deferSlowPathCacheMiss(sljit_jump* enter, u32 paddr) -> void {
  auto& slow = slowPaths.emplace_back();
  slow.enters.push_back(enter);
  slow.resume = sljit_emit_label(compiler);
  slow.icacheMiss = true;
  slow.icachePaddr = paddr;
  slow.vaddr = emitVaddr;
  slow.deferredCycles = emitDeferredCycles;
}

auto CPU::Recompiler::jitMemoryOpcode(u32 instruction, u32 size, u32 mode,
  const std::function<EmitExecuteResult()>& fallback, bool emitSlowPath) -> EmitExecuteResult {
  bool sign      = mode & SignExtend;
  bool require64 = mode & Require64;
  bool store     = mode & Store;
  bool partialLeft = mode & PartialLeft;
  bool partialRight = mode & PartialRight;
  bool floating  = mode & Floating;
  bool linkedConditional = mode & LinkedConditional;
  bool reverseEndian = emitStateKey.reverseEndian();
  bool loadLinked = linkedConditional && !store;
  bool storeConditional = linkedConditional && store;
  u32 reverseEndianXor = 0;
  if(reverseEndian) {
    if(size == Byte) reverseEndianXor = 7;
    if(size == Half) reverseEndianXor = 6;
    if(size == Word) reverseEndianXor = 4;
  }
  s32 floatingWordOff = 0;
  s32 floatingDualOff = 0;
  if(floating) {
    floatingDualOff = emitStateKey.floatingPointMode() ? Ftn * sizeof(r64) : (Ftn & ~1) * sizeof(r64);
    if(emitStateKey.floatingPointMode()) {
      floatingWordOff = Ftn * sizeof(r64) + FpuR64S32Off;
    } else {
      floatingWordOff = (Ftn & ~1) * sizeof(r64);
      floatingWordOff += Ftn & 1 ? FpuR64S32hOff : FpuR64S32Off;
    }
  }
  if(emitSlowPath || emitStateKey.watchpointsActive() || (require64 && reservedInstruction64())
  || (store && size == Dual && (partialLeft || partialRight) && system.homebrewMode)
  || (floating && !emitStateKey.coprocessor1Enabled())) {
    return fallback();
  }

  // The state key lets us specialize the virtual address checks for the current address width.
  auto extendedAddressing = [&] {
    if(emitStateKey.exceptionLevel() || emitStateKey.errorLevel()) return emitStateKey.kernelExtendedAddressing();
    auto privilegeMode = emitStateKey.privilegeMode();
    if(privilegeMode == 1) return emitStateKey.supervisorExtendedAddressing();
    if(privilegeMode >= 2) return emitStateKey.userExtendedAddressing();
    return emitStateKey.kernelExtendedAddressing();
  }();

  // Compute the virtual address and reject anything outside cached RDRAM, or not 32-bit sign-extended.
  bool gpBase = Rsn == 28;
  bool spBase = Rsn == 29;
  bool rangeKnown = gpBase && emitStateKey.gpCachedRdramOff16();
  auto alignmentKnown = [&] {
    if(size == Byte) return true;
    bool aligned4 = gpBase ? emitStateKey.gpAligned4() : spBase && emitStateKey.spAligned4();
    bool aligned8 = gpBase ? emitStateKey.gpAligned8() : spBase && emitStateKey.spAligned8();
    if(size == Half) return aligned4 && (i16 & 1) == 0;
    if(size == Word) return aligned4 && (i16 & 3) == 0;
    if(size == Dual) return aligned8 && (i16 & 7) == 0;
    return false;
  }();
  add64(reg(0), mem(Rs), imm(i16));
  sljit_jump* addressMismatch = nullptr;
  sljit_jump* addressOutOfRange = nullptr;
  if(!rangeKnown) {
    if(extendedAddressing) {
      sub64(reg(1), reg(0), imm((sljit_sw)0xffff'ffff'8000'0000ull));
      cmp64(reg(1), imm(0x007f'ffff), set_ugt);
    } else {
      mov64_s32(reg(1), reg(0));
      cmp64(reg(0), reg(1), set_z);
    }
    addressMismatch = jump(extendedAddressing ? flag_ugt : flag_nz);
    if(!extendedAddressing) {
      sub32(reg(1), reg(0), imm((sljit_sw)0x8000'0000u));
      cmp32(reg(1), imm(0x007f'ffff), set_ugt);
    }
    addressOutOfRange = !extendedAddressing ? jump(flag_ugt) : nullptr;
  }

  // Hardware raises address errors before any cache lookup on unaligned accesses.
  sljit_jump* addressUnaligned = nullptr;
  if(!(partialLeft || partialRight) && size > Byte && !alignmentKnown) {
    test32(reg(0), imm(size - 1), set_z);
    addressUnaligned = jump(flag_nz);
  }

  // Convert the cached virtual address to an RDRAM physical address and locate its dcache line.
  and32(reg(0), reg(0), imm(0x007f'ffff));
  if(reverseEndianXor) {
    xor32(reg(0), reg(0), imm(reverseEndianXor));
  }
  lshr32(reg(1), reg(0), imm(4));
  and32(reg(1), reg(1), imm(0x1ff));
  mul64(reg(2), reg(1), imm(CpuDcacheLineBytes));
  add64(reg(2), reg(2), imm(CpuDcacheLine0Off));
  add64(reg(2), reg(2), sreg(0));

  // Compare tag and valid bit together; misses fall back to the interpreter to fill/write back.
  and32(reg(3), reg(0), imm((sljit_sw)0xffff'f000u));
  or32(reg(3), reg(3), imm(1));
  cmp32(mem(reg(2), DcacheLineTagKeyOff), reg(3), set_z);
  auto cacheMiss = jump(flag_nz);

  // Cache hit: account for the hit latency, then read/write the cached value.
  emitCpuStep(2);
  if(system.homebrewMode) {
    add64(ProfileDcacheHitsMem, ProfileDcacheHitsMem, imm(1));
  }
  if(loadLinked) {
    lshr32(reg(4), reg(0), imm(4));
    mov32(SccLl, reg(4));
    mov32_u8(SccLlbit, imm(1));
  }
  if(store) {
    if(storeConditional && size == Word) {
      and32(reg(3), reg(0), imm(0x0c));
      add64(reg(3), reg(2), reg(3));
      mov32(reg(0), mem(reg(3), DcacheLineWordsOff));
      mov32(reg(1), mem(Rt32));
      mov32_u8(reg(4), SccLlbit);
      and32(reg(4), reg(4), imm(1));
      mov32(reg(5), imm(0));
      sub32(reg(5), reg(5), reg(4));
      and32(reg(1), reg(1), reg(5));
      xor32(reg(5), reg(5), imm((sljit_sw)0xffff'ffffu));
      and32(reg(0), reg(0), reg(5));
      or32(reg(0), reg(0), reg(1));
      mov32(mem(reg(3), DcacheLineWordsOff), reg(0));
      mov32(reg(5), reg(4));
      if(Rtn != 0) mov64(mem(Rt), reg(4));
    } else if(storeConditional && size == Dual) {
      and32(reg(3), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(3));
      mov32(reg(0), mem(reg(3), DcacheLineWordsOff + 0));
      mov32(reg(1), mem(reg(3), DcacheLineWordsOff + 4));
      mov32_u8(reg(4), SccLlbit);
      and32(reg(4), reg(4), imm(1));
      mov64(reg(5), mem(Rt));
      lshr64(reg(5), reg(5), imm(32));
      mov32(reg(5), reg(5));
      cmp32(reg(4), imm(0), set_z);
      cmov32(reg(0), reg(5), reg(0), flag_nz);
      mov32(reg(5), mem(Rt32));
      cmov32(reg(1), reg(5), reg(1), flag_nz);
      mov32(mem(reg(3), DcacheLineWordsOff + 0), reg(0));
      mov32(mem(reg(3), DcacheLineWordsOff + 4), reg(1));
      mov32(reg(5), reg(4));
      if(Rtn != 0) mov64(mem(Rt), reg(4));
    } else if(floating && size == Word) {
      and32(reg(3), reg(0), imm(0x0c));
      add64(reg(3), reg(2), reg(3));
      mov32(reg(0), mem(sreg(2), offsetof(FPU, r[0]) - FpuBase + floatingWordOff));
      mov32(mem(reg(3), DcacheLineWordsOff), reg(0));
    } else if(floating && size == Dual) {
      and32(reg(3), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(3));
      mov64(reg(0), mem(sreg(2), offsetof(FPU, r[0]) - FpuBase + floatingDualOff));
      lshr64(reg(1), reg(0), imm(32));
      mov32(mem(reg(3), DcacheLineWordsOff + 0), reg(1));
      mov32(mem(reg(3), DcacheLineWordsOff + 4), reg(0));
    } else if(partialLeft && size == Word) {
      and32(reg(1), reg(0), imm(3));
      if(reverseEndian) xor32(reg(1), reg(1), imm(3));
      shl32(reg(1), reg(1), imm(3));
      and32(reg(0), reg(0), imm(0x0c));
      add64(reg(0), reg(2), reg(0));
      mov32(reg(3), imm((sljit_sw)0xffff'ffffu));
      lshr32(reg(3), reg(3), reg(1));
      lshr32(reg(1), mem(Rt32), reg(1));
      xor32(reg(3), reg(3), imm((sljit_sw)0xffff'ffffu));
      and32(reg(3), mem(reg(0), DcacheLineWordsOff), reg(3));
      or32(reg(1), reg(1), reg(3));
      mov32(mem(reg(0), DcacheLineWordsOff), reg(1));
    } else if(partialRight && size == Word) {
      and32(reg(4), reg(0), imm(3));
      if(!reverseEndian) xor32(reg(4), reg(4), imm(3));
      shl32(reg(4), reg(4), imm(3));
      and32(reg(3), reg(0), imm(0x0c));
      add64(reg(3), reg(2), reg(3));
      mov32(reg(0), imm(1));
      shl32(reg(0), reg(0), reg(4));
      sub32(reg(0), reg(0), imm(1));
      mov32(reg(1), mem(reg(3), DcacheLineWordsOff));
      and32(reg(1), reg(1), reg(0));
      shl32(reg(5), mem(Rt32), reg(4));
      or32(reg(1), reg(1), reg(5));
      mov32(mem(reg(3), DcacheLineWordsOff), reg(1));
    } else if(partialLeft && size == Dual) {
      and32(reg(1), reg(0), imm(7));
      if(reverseEndian) xor32(reg(1), reg(1), imm(7));
      shl32(reg(1), reg(1), imm(3));
      and32(reg(0), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(0));
      mov64(reg(0), mem(reg(3), DcacheLineWordsOff));
      rotr64(reg(0), reg(0), imm(32));
      mov64(reg(4), imm((sljit_sw)-1));
      lshr64(reg(4), reg(4), reg(1));
      lshr64(reg(5), mem(Rt), reg(1));
      and64(reg(5), reg(5), reg(4));
      xor64(reg(4), reg(4), imm((sljit_sw)-1));
      and64(reg(0), reg(0), reg(4));
      or64(reg(0), reg(0), reg(5));
      lshr64(reg(1), reg(0), imm(32));
      mov32(mem(reg(3), DcacheLineWordsOff + 0), reg(1));
      mov32(mem(reg(3), DcacheLineWordsOff + 4), reg(0));
    } else if(partialRight && size == Dual) {
      and32(reg(1), reg(0), imm(7));
      if(!reverseEndian) xor32(reg(1), reg(1), imm(7));
      shl32(reg(1), reg(1), imm(3));
      and32(reg(0), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(0));
      mov64(reg(0), mem(reg(3), DcacheLineWordsOff));
      rotr64(reg(0), reg(0), imm(32));
      mov64(reg(4), imm(1));
      shl64(reg(4), reg(4), reg(1));
      sub64(reg(4), reg(4), imm(1));
      and64(reg(0), reg(0), reg(4));
      shl64(reg(5), mem(Rt), reg(1));
      xor64(reg(4), reg(4), imm((sljit_sw)-1));
      and64(reg(5), reg(5), reg(4));
      or64(reg(0), reg(0), reg(5));
      lshr64(reg(1), reg(0), imm(32));
      mov32(mem(reg(3), DcacheLineWordsOff + 0), reg(1));
      mov32(mem(reg(3), DcacheLineWordsOff + 4), reg(0));
    } else if(size == Byte) {
      and32(reg(3), reg(0), imm(0x0f));
      xor32(reg(3), reg(3), imm(3));
      add64(reg(3), reg(2), reg(3));
      mov32_u8(mem(reg(3), DcacheLineWordsOff), mem(Rt32));
    } else if(size == Half) {
      and32(reg(3), reg(0), imm(0x0e));
      xor32(reg(3), reg(3), imm(2));
      add64(reg(3), reg(2), reg(3));
      mov32_u16(mem(reg(3), DcacheLineWordsOff), mem(Rt32));
    } else if(size == Word) {
      and32(reg(3), reg(0), imm(0x0c));
      add64(reg(3), reg(2), reg(3));
      mov32(mem(reg(3), DcacheLineWordsOff), mem(Rt32));
    } else if(size == Dual) {
      and32(reg(3), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(3));
      mov64(reg(0), mem(Rt));
      lshr64(reg(0), reg(0), imm(32));
      mov32(mem(reg(3), DcacheLineWordsOff + 0), reg(0));
      mov32(mem(reg(3), DcacheLineWordsOff + 4), mem(Rt32));
    }

    if(system.homebrewMode) {
      add64(reg(4), mem(Rs), imm(i16));
      and32(reg(4), reg(4), imm(0x0f));
      if(reverseEndianXor) {
        xor32(reg(4), reg(4), imm(reverseEndianXor));
      }
      if(partialLeft && size == Word) {
        and32(reg(3), reg(4), imm(3));
        if(reverseEndian) xor32(reg(3), reg(3), imm(3));
        mov32(reg(1), imm(0x0f));
        lshr32(reg(1), reg(1), reg(3));
        and32(reg(3), reg(4), imm(0x0c));
        shl32(reg(1), reg(1), reg(3));
      } else if(partialRight && size == Word) {
        and32(reg(3), reg(4), imm(3));
        if(reverseEndian) xor32(reg(3), reg(3), imm(3));
        add32(reg(3), reg(3), imm(1));
        mov32(reg(1), imm(1));
        shl32(reg(1), reg(1), reg(3));
        sub32(reg(1), reg(1), imm(1));
        and32(reg(3), reg(4), imm(0x0c));
        shl32(reg(1), reg(1), reg(3));
      } else {
        mov32(reg(1), imm((1 << size) - 1));
        shl32(reg(1), reg(1), reg(4));
      }
      mov32_u16(reg(0), mem(reg(2), DcacheLineDirtyOff));
      if(storeConditional && (size == Word || size == Dual)) {
        mov32(reg(4), imm(0));
        sub32(reg(4), reg(4), reg(5));
        and32(reg(1), reg(1), reg(4));
      }
      or32(reg(1), reg(1), reg(0));
      mov32_u16(mem(reg(2), DcacheLineDirtyOff), reg(1));
      if(storeConditional && (size == Word || size == Dual)) {
        mov64(reg(0), mem(reg(2), DcacheLineDirtyPcOff));
        mov64(reg(1), imm(emitVaddr));
        cmp32(reg(5), imm(0), set_z);
        cmov64(reg(0), reg(1), reg(0), flag_nz);
        mov64(mem(reg(2), DcacheLineDirtyPcOff), reg(0));
      } else {
        mov64(mem(reg(2), DcacheLineDirtyPcOff), imm(emitVaddr));
      }
    } else {
      if(storeConditional && (size == Word || size == Dual)) {
        mov32_u16(reg(0), mem(reg(2), DcacheLineDirtyOff));
        mov32(reg(1), reg(5));
        or32(reg(1), reg(1), reg(0));
        mov32_u16(mem(reg(2), DcacheLineDirtyOff), reg(1));
      } else {
        mov32_u16(mem(reg(2), DcacheLineDirtyOff), imm(1));
      }
    }
    if(storeConditional && (size == Word || size == Dual)) mov32_u8(SccLlbit, imm(0));
  } else if(floating) {
    and32(reg(3), reg(0), imm(size == Dual ? 0x08 : 0x0c));
    add64(reg(3), reg(2), reg(3));
    if(size == Dual) {
      mov64(reg(3), mem(reg(3), DcacheLineWordsOff));
      rotr64(reg(3), reg(3), imm(32));
      mov64(mem(sreg(2), offsetof(FPU, r[0]) - FpuBase + floatingDualOff), reg(3));
    } else {
      mov32(reg(3), mem(reg(3), DcacheLineWordsOff));
      mov32(mem(sreg(2), offsetof(FPU, r[0]) - FpuBase + floatingWordOff), reg(3));
    }
  } else if(Rtn != 0) {
    if(partialLeft && size == Word) {
      and32(reg(3), reg(0), imm(0x0c));
      add64(reg(3), reg(2), reg(3));
      mov32(reg(3), mem(reg(3), DcacheLineWordsOff));
      and32(reg(1), reg(0), imm(3));
      if(reverseEndian) xor32(reg(1), reg(1), imm(3));
      shl32(reg(1), reg(1), imm(3));
      shl32(reg(3), reg(3), reg(1));
      mov32(reg(0), imm(1));
      shl32(reg(0), reg(0), reg(1));
      sub32(reg(0), reg(0), imm(1));
      and32(reg(0), mem(Rt32), reg(0));
      or32(reg(3), reg(3), reg(0));
      mov64_s32(reg(3), reg(3));
    } else if(partialLeft && size == Dual) {
      and32(reg(1), reg(0), imm(7));
      if(reverseEndian) xor32(reg(1), reg(1), imm(7));
      shl32(reg(1), reg(1), imm(3));
      and32(reg(3), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(3));
      mov64(reg(3), mem(reg(3), DcacheLineWordsOff));
      rotr64(reg(3), reg(3), imm(32));
      shl64(reg(3), reg(3), reg(1));
      mov64(reg(0), imm(1));
      shl64(reg(0), reg(0), reg(1));
      sub64(reg(0), reg(0), imm(1));
      and64(reg(0), mem(Rt), reg(0));
      or64(reg(3), reg(3), reg(0));
    } else if(partialRight && size == Word) {
      and32(reg(3), reg(0), imm(0x0c));
      add64(reg(3), reg(2), reg(3));
      mov32(reg(3), mem(reg(3), DcacheLineWordsOff));
      and32(reg(1), reg(0), imm(3));
      if(!reverseEndian) xor32(reg(1), reg(1), imm(3));
      shl32(reg(1), reg(1), imm(3));
      lshr32(reg(3), reg(3), reg(1));
      mov32(reg(2), imm((sljit_sw)0xffff'ffffu));
      lshr32(reg(0), reg(2), reg(1));
      xor32(reg(0), reg(0), imm((sljit_sw)0xffff'ffffu));
      and32(reg(0), mem(Rt32), reg(0));
      or32(reg(3), reg(3), reg(0));
      mov64_s32(reg(2), reg(3));
      lshr64(reg(0), mem(Rt), imm(32));
      shl64(reg(0), reg(0), imm(32));
      or64(reg(3), reg(3), reg(0));
      cmp32(reg(1), imm(0), set_z);
      cmov64(reg(3), reg(2), reg(3), flag_z);
    } else if(partialRight && size == Dual) {
      and32(reg(1), reg(0), imm(7));
      if(!reverseEndian) xor32(reg(1), reg(1), imm(7));
      shl32(reg(1), reg(1), imm(3));
      and32(reg(3), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(3));
      mov64(reg(3), mem(reg(3), DcacheLineWordsOff));
      rotr64(reg(3), reg(3), imm(32));
      lshr64(reg(3), reg(3), reg(1));
      mov64(reg(2), imm((sljit_sw)-1));
      lshr64(reg(0), reg(2), reg(1));
      xor64(reg(0), reg(0), imm((sljit_sw)-1));
      and64(reg(0), mem(Rt), reg(0));
      or64(reg(3), reg(3), reg(0));
    } else if(size == Byte) {
      and32(reg(3), reg(0), imm(0x0f));
      xor32(reg(3), reg(3), imm(3));
      add64(reg(3), reg(2), reg(3));
      if(sign) mov64_s8(reg(3), mem(reg(3), DcacheLineWordsOff));
      else     mov64_u8(reg(3), mem(reg(3), DcacheLineWordsOff));
    } else if(size == Half) {
      and32(reg(3), reg(0), imm(0x0e));
      xor32(reg(3), reg(3), imm(2));
      add64(reg(3), reg(2), reg(3));
      if(sign) mov64_s16(reg(3), mem(reg(3), DcacheLineWordsOff));
      else     mov64_u16(reg(3), mem(reg(3), DcacheLineWordsOff));
    } else if(size == Word) {
      and32(reg(3), reg(0), imm(0x0c));
      add64(reg(3), reg(2), reg(3));
      if(sign) mov64_s32(reg(3), mem(reg(3), DcacheLineWordsOff));
      else     mov64_u32(reg(3), mem(reg(3), DcacheLineWordsOff));
    } else if(size == Dual) {
      and32(reg(3), reg(0), imm(0x08));
      add64(reg(3), reg(2), reg(3));
      mov64(reg(3), mem(reg(3), DcacheLineWordsOff));
      rotr64(reg(3), reg(3), imm(32));
    }
    mov64(mem(Rt), reg(3));
  }

  // All failed fast-path guards share one generated slow path and return here afterwards.
  deferSlowPath({addressMismatch, addressOutOfRange, addressUnaligned, cacheMiss}, instruction);
  return EmitExecuteResult::Linear;
}


auto CPU::Recompiler::emitEXECUTE(u32 instruction, bool emitSlowPath, EmitPcMode pcMode) -> EmitExecuteResult {
  emitSlowPathSection = emitSlowPath;
  auto emitJumpTarget = [&](u32 target) -> void {
    if(pcMode == EmitPcMode::Runtime) {
      and64(reg(0), PipelineReg(pc), imm(0xffff'ffff'f000'0000ull));
      or64(reg(0), reg(0), imm(target << 2));
      mov64(PipelineReg(nextpc), reg(0));
      return;
    }
    mov64(PipelineReg(nextpc), imm(((emitVaddr + 4) & 0xffff'ffff'f000'0000ull) | (u64(target) << 2)));
  };
  auto emitBranchTargetReg = [&](reg target, s16 offset) -> void {
    if(pcMode == EmitPcMode::Runtime) {
      add64(target, PipelineReg(pc), imm(s32(offset) * 4));
      return;
    }
    mov64(target, imm(emitVaddr + 4 + s64(s32(offset) * 4)));
  };
  switch(instruction >> 26) {

  //SPECIAL
  case 0x00: {
    return emitSPECIAL(instruction);
  }

  //REGIMM
  case 0x01: {
    return emitREGIMM(instruction, pcMode);
  }

  //J n26
  case 0x02: {
    emitJumpTarget(n26);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    return EmitExecuteResult::MayBranch;
  }

  //JAL n26
  case 0x03: {
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(1), PipelineReg(pc), imm(4));
      mov64(mem(IpuReg(r[31])), reg(1));
    } else {
      mov64(mem(IpuReg(r[31])), imm(emitVaddr + 8));
    }
    emitJumpTarget(n26);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    return EmitExecuteResult::MayBranch;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), mem(Rt), set_z);
    cmov64(reg(2), reg(1), reg(0), flag_z);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_z);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), mem(Rt), set_z);
    cmov64(reg(2), reg(1), reg(0), flag_nz);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_nz);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BLEZ Rs,i16
  case 0x06: {
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), imm(0), set_sgt);
    cmov64(reg(2), reg(1), reg(0), flag_sle);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sle);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BGTZ Rs,i16
  case 0x07: {
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), imm(0), set_sgt);
    cmov64(reg(2), reg(1), reg(0), flag_sgt);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sgt);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //ADDI Rt,Rs,i16
  case 0x08: {
    if(emitSlowPath) {
      setupCallf();
      callf(&CPU::ADDI, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    add32(reg(0), mem(Rs32), imm(i16), set_o);
    auto overflow = jump(flag_o);
    if(Rtn != 0) {
      mov64_s32(reg(0), reg(0));
      mov64(mem(Rt), reg(0));
    }
    deferSlowPath(overflow, instruction);
    return EmitExecuteResult::Linear;
  }

  //ADDIU Rt,Rs,i16
  case 0x09: {
    if(Rtn == 0) return EmitExecuteResult::Linear;
    add32(reg(0), mem(Rs32), imm(i16));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rt), reg(0));
    if(Rtn == 29 && Rsn == 29) updateStackPointerStateKey(i16);
    return EmitExecuteResult::Linear;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    if(Rtn == 0) return EmitExecuteResult::Linear;
    cmp64(mem(Rs), imm(i16), set_slt);
    mov64_f(mem(Rt), flag_slt);
    return EmitExecuteResult::Linear;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    if(Rtn == 0) return EmitExecuteResult::Linear;
    cmp64(mem(Rs), imm(i16), set_ult);
    mov64_f(mem(Rt), flag_ult);
    return EmitExecuteResult::Linear;
  }

  //ANDI Rt,Rs,n16
  case 0x0c: {
    if(Rtn == 0) return EmitExecuteResult::Linear;
    and64(mem(Rt), mem(Rs), imm(n16));
    return EmitExecuteResult::Linear;
  }

  //ORI Rt,Rs,n16
  case 0x0d: {
    if(Rtn == 0) return EmitExecuteResult::Linear;
    or64(mem(Rt), mem(Rs), imm(n16));
    return EmitExecuteResult::Linear;
  }

  //XORI Rt,Rs,n16
  case 0x0e: {
    if(Rtn == 0) return EmitExecuteResult::Linear;
    xor64(mem(Rt), mem(Rs), imm(n16));
    return EmitExecuteResult::Linear;
  }

  //LUI Rt,n16
  case 0x0f: {
    if(Rtn == 0) return EmitExecuteResult::Linear;
    mov64(mem(Rt), imm(s32(n16 << 16)));
    return EmitExecuteResult::Linear;
  }

  //SCC
  case 0x10: {
    return emitSCC(instruction, pcMode);
  }

  //FPU
  case 0x11: {
    return emitFPU(instruction, pcMode);
  }

  //COP2
  case 0x12: {
    return emitCOP2(instruction);
  }

  //COP3
  case 0x13: {
    setupCallf();
    callf(&CPU::COP3);
    return EmitExecuteResult::MayFault;
  }

  //BEQL Rs,Rt,i16
  case 0x14: {
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), mem(Rt), set_z);
    cmov64(reg(0), reg(0), reg(3), flag_z);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_z);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_z);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_z);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BNEL Rs,Rt,i16
  case 0x15: {
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), mem(Rt), set_z);
    cmov64(reg(0), reg(0), reg(3), flag_nz);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_nz);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_nz);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_nz);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BLEZL Rs,i16
  case 0x16: {
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), imm(0), set_sgt);
    cmov64(reg(0), reg(0), reg(3), flag_sle);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_sle);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_sle);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sle);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BGTZL Rs,i16
  case 0x17: {
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), imm(0), set_sgt);
    cmov64(reg(0), reg(0), reg(3), flag_sgt);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_sgt);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_sgt);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sgt);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //DADDI Rt,Rs,i16
  case 0x18: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPath || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADDI, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    add64(reg(0), mem(Rs), imm(i16), set_o);
    auto overflow = jump(flag_o);
    if(Rtn != 0) mov64(mem(Rt), reg(0));
    deferSlowPath(overflow, instruction);
    return EmitExecuteResult::Linear;
  }

  //DADDIU Rt,Rs,i16
  case 0x19: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPath || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADDIU, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    add64(reg(0), mem(Rs), imm(i16));
    if(Rtn != 0) mov64(mem(Rt), reg(0));
    if(Rtn == 29 && Rsn == 29) updateStackPointerStateKey(i16);
    return EmitExecuteResult::Linear;
  }

  //LDL Rt,Rs,i16
  case 0x1a: {
    return jitMemoryOpcode(instruction, Dual, Require64 | PartialLeft, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LDL, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LDR Rt,Rs,i16
  case 0x1b: {
    return jitMemoryOpcode(instruction, Dual, Require64 | PartialRight, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LDR, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //INVALID
  case range4(0x1c, 0x1f): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    return jitMemoryOpcode(instruction, Byte, SignExtend, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LB, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LH Rt,Rs,i16
  case 0x21: {
    return jitMemoryOpcode(instruction, Half, SignExtend, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LH, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LWL Rt,Rs,i16
  case 0x22: {
    return jitMemoryOpcode(instruction, Word, SignExtend | PartialLeft, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LWL, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }
  //LW Rt,Rs,i16
  case 0x23: {
    return jitMemoryOpcode(instruction, Word, SignExtend, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LW, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    return jitMemoryOpcode(instruction, Byte, 0, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LBU, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    return jitMemoryOpcode(instruction, Half, 0, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LHU, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LWR Rt,Rs,i16
  case 0x26: {
    return jitMemoryOpcode(instruction, Word, SignExtend | PartialRight, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LWR, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LWU Rt,Rs,i16
  case 0x27: {
    return jitMemoryOpcode(instruction, Word, 0, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LWU, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SB Rt,Rs,i16
  case 0x28: {
    return jitMemoryOpcode(instruction, Byte, Store, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SB, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SH Rt,Rs,i16
  case 0x29: {
    return jitMemoryOpcode(instruction, Half, Store, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SH, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SWL Rt,Rs,i16
  case 0x2a: {
    return jitMemoryOpcode(instruction, Word, Store | PartialLeft, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SWL, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    return jitMemoryOpcode(instruction, Word, Store, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SW, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SDL Rt,Rs,i16
  case 0x2c: {
    return jitMemoryOpcode(instruction, Dual, Store | PartialLeft | Require64, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SDL, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SDR Rt,Rs,i16
  case 0x2d: {
    return jitMemoryOpcode(instruction, Dual, Store | PartialRight | Require64, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SDR, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SWR Rt,Rs,i16
  case 0x2e: {
    return jitMemoryOpcode(instruction, Word, Store | PartialRight, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SWR, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //CACHE op(offset),base
  case 0x2f: {
    setupCallf();
    callf(&CPU::CACHE, imm(instruction >> 16 & 31), mem(Rs), imm(i16));
    return EmitExecuteResult::MayFault;
  }

  //LL Rt,Rs,i16
  case 0x30: {
    return jitMemoryOpcode(instruction, Word, SignExtend | LinkedConditional, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LL, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LWC1 Ft,Rs,i16
  case 0x31: {
    return jitMemoryOpcode(instruction, Word, Floating, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LWC1, imm(Ftn), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LWC2
  case 0x32: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return EmitExecuteResult::MayFault;
  }

  //LWC3
  case 0x33: {
    setupCallf();
    callf(&CPU::COP3);
    return EmitExecuteResult::MayFault;
  }

  //LLD Rt,Rs,i16
  case 0x34: {
    return jitMemoryOpcode(instruction, Dual, Require64 | LinkedConditional, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LLD, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LDC1 Ft,Rs,i16
  case 0x35: {
    return jitMemoryOpcode(instruction, Dual, Floating, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LDC1, imm(Ftn), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //LDC2
  case 0x36: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return EmitExecuteResult::MayFault;
  }

  //LD Rt,Rs,i16
  case 0x37: {
    return jitMemoryOpcode(instruction, Dual, Require64, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::LD, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SC Rt,Rs,i16
  case 0x38: {
    return jitMemoryOpcode(instruction, Word, Store | LinkedConditional, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SC, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SWC1 Ft,Rs,i16
  case 0x39: {
    return jitMemoryOpcode(instruction, Word, Store | Floating, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SWC1, imm(Ftn), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SWC2
  case 0x3a: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return EmitExecuteResult::MayFault;
  }

  //SWC3
  case 0x3b: {
    setupCallf();
    callf(&CPU::COP3);
    return EmitExecuteResult::MayFault;
  }

  //SCD Rt,Rs,i16
  case 0x3c: {
    return jitMemoryOpcode(instruction, Dual, Require64 | Store | LinkedConditional, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SCD, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SDC1 Ft,Rs,i16
  case 0x3d: {
    return jitMemoryOpcode(instruction, Dual, Store | Floating, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SDC1, imm(Ftn), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  //SDC2
  case 0x3e: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return EmitExecuteResult::MayFault;
  }

  //SD Rt,Rs,i16
  case 0x3f: {
    return jitMemoryOpcode(instruction, Dual, Require64 | Store, [&]() -> EmitExecuteResult {
      setupCallf();
      callf(&CPU::SD, mem(Rt), mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }, emitSlowPath);
  }

  }

  return EmitExecuteResult::Linear;
}

auto CPU::Recompiler::emitSPECIAL(u32 instruction) -> EmitExecuteResult {
  switch(instruction & 0x3f) {

  //SLL Rd,Rt,Sa
  case 0x00: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    shl32(reg(0), mem(Rt32), imm(Sa));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x01: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    lshr32(reg(0), mem(Rt32), imm(Sa));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    ashr64(reg(0), mem(Rt), imm(Sa));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    mshl32(reg(0), mem(Rt32), mem(Rs32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x05: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //SRLV Rd,Rt,RS
  case 0x06: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    mlshr32(reg(0), mem(Rt32), mem(Rs32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    and64(reg(1), mem(Rs), imm(31));
    ashr64(reg(0), mem(Rt), reg(1));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //JR Rs
  case 0x08: {
    mov64(PipelineReg(nextpc), mem(Rs));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    return EmitExecuteResult::MayBranch;
  }

  //JALR Rd,Rs
  case 0x09: {
    mov64(reg(1), mem(Rs));
    if(emitPcMode == EmitPcMode::Runtime) {
      add64(reg(0), PipelineReg(pc), imm(4));
      if(Rdn) mov64(mem(Rd), reg(0));
    } else {
      if(Rdn) mov64(mem(Rd), imm(emitVaddr + 8));
    }
    mov64(PipelineReg(nextpc), reg(1));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    return EmitExecuteResult::MayBranch;
  }

  //INVALID
  case range2(0x0a, 0x0b): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //SYSCALL
  case 0x0c: {
    setupCallf();
    callf(&CPU::SYSCALL);
    return EmitExecuteResult::MayFault;
  }

  //BREAK
  case 0x0d: {
    setupCallf();
    callf(&CPU::BREAK);
    return EmitExecuteResult::MayFault;
  }

  //INVALID
  case 0x0e: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //SYNC
  case 0x0f: {
    // no operation
    return EmitExecuteResult::Linear;
  }

  //MFHI Rd
  case 0x10: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    mov64(mem(Rd), mem(Hi));
    return EmitExecuteResult::Linear;
  }

  //MTHI Rs
  case 0x11: {
    mov64(mem(Hi), mem(Rs));
    return EmitExecuteResult::Linear;
  }

  //MFLO Rd
  case 0x12: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    mov64(mem(Rd), mem(Lo));
    return EmitExecuteResult::Linear;
  }

  //MTLO Rs
  case 0x13: {
    mov64(mem(Lo), mem(Rs));
    return EmitExecuteResult::Linear;
  }

  //DSLLV Rd,Rt,Rs
  case 0x14: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSLLV, mem(Rd), mem(Rt), mem(Rs));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    and64(reg(1), mem(Rs), imm(63));
    shl64(reg(0), mem(Rt), reg(1));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x15: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //DSRLV Rd,Rt,Rs
  case 0x16: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRLV, mem(Rd), mem(Rt), mem(Rs));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    and64(reg(1), mem(Rs), imm(63));
    lshr64(reg(0), mem(Rt), reg(1));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //DSRAV Rd,Rt,Rs
  case 0x17: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRAV, mem(Rd), mem(Rt), mem(Rs));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    and64(reg(1), mem(Rs), imm(63));
    ashr64(reg(0), mem(Rt), reg(1));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //MULT Rs,Rt
  case 0x18: {
    mov64(reg(0), mem(Rt));
    shl64(reg(0), reg(0), imm(29));
    ashr64(reg(0), reg(0), imm(29));
    mul64(reg(0), mem(Rs), reg(0));
    mov64_s32(reg(1), reg(0));
    mov64(mem(Lo), reg(1));
    ashr64(reg(1), reg(0), imm(32));
    mov64(mem(Hi), reg(1));
    emitDeferredCycles += (5 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //MULTU Rs,Rt
  case 0x19: {
    mov64_u32(reg(0), mem(Rs32));
    mov64_u32(reg(1), mem(Rt32));
    mul64(reg(0), reg(0), reg(1));
    mov64_s32(reg(1), reg(0));
    mov64(mem(Lo), reg(1));
    ashr64(reg(1), reg(0), imm(32));
    mov64(mem(Hi), reg(1));
    emitDeferredCycles += (5 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //DIV Rs,Rt
  case 0x1a: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::DIV, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rt), imm(0), set_z);
    auto divByZero = jump(flag_z);
    mov64_s32(reg(2), mem(Rs32));
    divmod64_sw(reg(0), reg(1), reg(2), mem(Rt));
    mov64_s32(reg(0), reg(0));
    mov64_s32(reg(1), reg(1));
    mov64(mem(Lo), reg(0));
    mov64(mem(Hi), reg(1));
    deferSlowPath(divByZero, instruction);
    emitDeferredCycles += (37 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //DIVU Rs,Rt
  case 0x1b: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::DIVU, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp32(mem(Rt32), imm(0), set_z);
    auto divByZero = jump(flag_z);
    divmod32_uw(reg(0), reg(1), mem(Rs32), mem(Rt32));
    mov64(mem(Lo), reg(0));
    mov64(mem(Hi), reg(1));
    deferSlowPath(divByZero, instruction);
    emitDeferredCycles += (37 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //DMULT Rs,Rt
  case 0x1c: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DMULT, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    lmul64_sw(mem(Lo), mem(Hi), mem(Rs), mem(Rt));
    emitDeferredCycles += (8 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //DMULTU Rs,Rt
  case 0x1d: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DMULTU, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    lmul64_uw(mem(Lo), mem(Hi), mem(Rs), mem(Rt));
    emitDeferredCycles += (8 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //DDIV Rs,Rt
  case 0x1e: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DDIV, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rt), imm(0), set_z);
    auto divByZero = jump(flag_z);
    cmp64(mem(Rs), imm((sljit_sw)0x8000'0000'0000'0000ull), set_z);
    auto rsNotMin = jump(flag_ne);
    cmp64(mem(Rt), imm(-1), set_z);
    auto notSpecial = jump(flag_ne);
    mov64(mem(Lo), mem(Rs));
    mov64(mem(Hi), imm(0));
    auto afterDdiv = jump();
    setLabel(rsNotMin);
    setLabel(notSpecial);
    divmod64_sw(mem(Lo), mem(Hi), mem(Rs), mem(Rt));
    setLabel(afterDdiv);
    deferSlowPath(divByZero, instruction);
    emitDeferredCycles += (69 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //DDIVU Rs,Rt
  case 0x1f: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DDIVU, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rt), imm(0), set_z);
    auto divByZero = jump(flag_z);
    divmod64_uw(mem(Lo), mem(Hi), mem(Rs), mem(Rt));
    deferSlowPath(divByZero, instruction);
    emitDeferredCycles += (69 - 1) * 2;
    return EmitExecuteResult::Linear;
  }

  //ADD Rd,Rs,Rt
  case 0x20: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::ADD, mem(Rd), mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    add32(reg(0), mem(Rs32), mem(Rt32), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) {
      mov64_s32(reg(0), reg(0));
      mov64(mem(Rd), reg(0));
    }
    deferSlowPath(overflow, instruction);
    return EmitExecuteResult::Linear;
  }

  //ADDU Rd,Rs,Rt
  case 0x21: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    add32(reg(0), mem(Rs32), mem(Rt32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //SUB Rd,Rs,Rt
  case 0x22: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::SUB, mem(Rd), mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    sub32(reg(0), mem(Rs32), mem(Rt32), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) {
      mov64_s32(reg(0), reg(0));
      mov64(mem(Rd), reg(0));
    }
    deferSlowPath(overflow, instruction);
    return EmitExecuteResult::Linear;
  }

  //SUBU Rd,Rs,Rt
  case 0x23: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    sub32(reg(0), mem(Rs32), mem(Rt32));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    and64(mem(Rd), mem(Rs), mem(Rt));
    return EmitExecuteResult::Linear;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    or64(mem(Rd), mem(Rs), mem(Rt));
    return EmitExecuteResult::Linear;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    xor64(mem(Rd), mem(Rs), mem(Rt));
    return EmitExecuteResult::Linear;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    or64(reg(0), mem(Rs), mem(Rt));
    xor64(reg(0), reg(0), imm(-1));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case range2(0x28, 0x29): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    cmp64(mem(Rs), mem(Rt), set_slt);
    mov64_f(mem(Rd), flag_slt);
    return EmitExecuteResult::Linear;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    if(Rdn == 0) return EmitExecuteResult::Linear;
    cmp64(mem(Rs), mem(Rt), set_ult);
    mov64_f(mem(Rd), flag_ult);
    return EmitExecuteResult::Linear;
  }

  //DADD Rd,Rs,Rt
  case 0x2c: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADD, mem(Rd), mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    add64(reg(0), mem(Rs), mem(Rt), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) mov64(mem(Rd), reg(0));
    deferSlowPath(overflow, instruction);
    return EmitExecuteResult::Linear;
  }

  //DADDU Rd,Rs,Rt
  case 0x2d: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADDU, mem(Rd), mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    add64(reg(0), mem(Rs), mem(Rt));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //DSUB Rd,Rs,Rt
  case 0x2e: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSUB, mem(Rd), mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    sub64(reg(0), mem(Rs), mem(Rt), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) mov64(mem(Rd), reg(0));
    deferSlowPath(overflow, instruction);
    return EmitExecuteResult::Linear;
  }

  //DSUBU Rd,Rs,Rt
  case 0x2f: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSUBU, mem(Rd), mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    sub64(reg(0), mem(Rs), mem(Rt));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //TGE Rs,Rt
  case 0x30: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGE, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), mem(Rt), set_slt);
    auto trap = jump(flag_sge);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TGEU Rs,Rt
  case 0x31: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGEU, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), mem(Rt), set_ult);
    auto trap = jump(flag_uge);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TLT Rs,Rt
  case 0x32: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLT, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), mem(Rt), set_slt);
    auto trap = jump(flag_slt);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TLTU Rs,Rt
  case 0x33: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLTU, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), mem(Rt), set_ult);
    auto trap = jump(flag_ult);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TEQ Rs,Rt
  case 0x34: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TEQ, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), mem(Rt), set_z);
    auto trap = jump(flag_z);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x35: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //TNE Rs,Rt
  case 0x36: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TNE, mem(Rs), mem(Rt));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), mem(Rt), set_z);
    auto trap = jump(flag_nz);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x37: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }
  //DSLL Rd,Rt,Sa
  case 0x38: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSLL, mem(Rd), mem(Rt), imm(Sa));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    shl64(reg(0), mem(Rt), imm(Sa));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x39: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //DSRL Rd,Rt,Sa
  case 0x3a: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRL, mem(Rd), mem(Rt), imm(Sa));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    lshr64(reg(0), mem(Rt), imm(Sa));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //DSRA Rd,Rt,Sa
  case 0x3b: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRA, mem(Rd), mem(Rt), imm(Sa));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    ashr64(reg(0), mem(Rt), imm(Sa));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //DSLL32 Rd,Rt,Sa
  case 0x3c: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSLL, mem(Rd), mem(Rt), imm(Sa+32));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    shl64(reg(0), mem(Rt), imm(Sa + 32));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x3d: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //DSRL32 Rd,Rt,Sa
  case 0x3e: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRL, mem(Rd), mem(Rt), imm(Sa+32));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    lshr64(reg(0), mem(Rt), imm(Sa + 32));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  //DSRA32 Rd,Rt,Sa
  case 0x3f: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRA, mem(Rd), mem(Rt), imm(Sa+32));
      return EmitExecuteResult::MayFault;
    }
    if(Rdn == 0) return EmitExecuteResult::Linear;
    ashr64(reg(0), mem(Rt), imm(Sa + 32));
    mov64(mem(Rd), reg(0));
    return EmitExecuteResult::Linear;
  }

  }

  return EmitExecuteResult::Linear;
}

auto CPU::Recompiler::emitREGIMM(u32 instruction, EmitPcMode pcMode) -> EmitExecuteResult {
  auto emitBranchTargetReg = [&](reg target, s16 offset) -> void {
    if(pcMode == EmitPcMode::Runtime) {
      add64(target, PipelineReg(pc), imm(s32(offset) * 4));
      return;
    }
    mov64(target, imm(emitVaddr + 4 + s64(s32(offset) * 4)));
  };
  auto emitLink31 = [&] {
    if(pcMode == EmitPcMode::Runtime) {
      add32(reg(0), PipelineReg(pc), imm(4));
      mov64_s32(reg(0), reg(0));
      mov64(mem(IpuReg(r[31])), reg(0));
      return;
    }
    mov64(mem(IpuReg(r[31])), imm(s64(s32(u32(emitVaddr + 8)))));
  };
  switch(instruction >> 16 & 0x1f) {

  //BLTZ Rs,i16
  case 0x00: {
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(2), reg(1), reg(0), flag_slt);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_slt);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BGEZ Rs,i16
  case 0x01: {
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(2), reg(1), reg(0), flag_sge);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sge);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BLTZL Rs,i16
  case 0x02: {
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(0), reg(0), reg(3), flag_slt);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_slt);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_slt);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_slt);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BGEZL Rs,i16
  case 0x03: {
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(0), reg(0), reg(3), flag_sge);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_sge);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_sge);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sge);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //INVALID
  case range4(0x04, 0x07): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //TGEI Rs,i16
  case 0x08: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGEI, mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), imm(i16), set_slt);
    auto trap = jump(flag_sge);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TGEIU Rs,i16
  case 0x09: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGEIU, mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), imm(i16), set_ult);
    auto trap = jump(flag_uge);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TLTI Rs,i16
  case 0x0a: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLTI, mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), imm(i16), set_slt);
    auto trap = jump(flag_slt);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TLTIU Rs,i16
  case 0x0b: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLTIU, mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), imm(i16), set_ult);
    auto trap = jump(flag_ult);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //TEQI Rs,i16
  case 0x0c: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TEQI, mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), imm(i16), set_z);
    auto trap = jump(flag_z);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x0d: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //TNEI Rs,i16
  case 0x0e: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TNEI, mem(Rs), imm(i16));
      return EmitExecuteResult::MayFault;
    }
    cmp64(mem(Rs), imm(i16), set_z);
    auto trap = jump(flag_nz);
    deferSlowPath(trap, instruction);
    return EmitExecuteResult::Linear;
  }

  //INVALID
  case 0x0f: {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    emitLink31();
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(2), reg(1), reg(0), flag_slt);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_slt);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    mov64(reg(0), PipelineReg(nextpc));
    emitBranchTargetReg(reg(1), i16);
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(2), reg(1), reg(0), flag_sge);
    mov64(PipelineReg(nextpc), reg(2));
    mov32(reg(0), imm(Pipeline::DelaySlot));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sge);
    mov32(PipelineReg(nstate), reg(2));
    emitLink31();
    return EmitExecuteResult::MayBranch;
  }

  //BLTZALL Rs,i16
  case 0x12: {
    emitLink31();
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(0), reg(0), reg(3), flag_slt);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_slt);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_slt);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_slt);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //BGEZALL Rs,i16
  case 0x13: {
    emitLink31();
    mov64(reg(0), PipelineReg(pc));
    mov64(reg(1), PipelineReg(state));
    emitBranchTargetReg(reg(2), i16);
    if(pcMode == EmitPcMode::Runtime) {
      add64(reg(3), reg(0), imm(4));
      add64(reg(4), reg(3), imm(4));
    } else {
      mov64(reg(3), imm(emitVaddr + 8));
      mov64(reg(4), imm(emitVaddr + 12));
    }
    or32(reg(5), reg(1), imm(Pipeline::EndBlock));
    cmp64(mem(Rs), imm(0), set_slt);
    cmov64(reg(0), reg(0), reg(3), flag_sge);
    mov64(PipelineReg(pc), reg(0));
    cmov64(reg(0), reg(2), reg(4), flag_sge);
    mov64(PipelineReg(nextpc), reg(0));
    cmov32(reg(0), reg(1), reg(5), flag_sge);
    mov32(PipelineReg(state), reg(0));
    mov32(reg(0), imm(0));
    mov32(reg(1), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    cmov32(reg(2), reg(1), reg(0), flag_sge);
    mov32(PipelineReg(nstate), reg(2));
    return EmitExecuteResult::MayBranch;
  }

  //INVALID
  case range12(0x14, 0x1f): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }
  }

  return EmitExecuteResult::Linear;
}

auto CPU::Recompiler::emitSCC(u32 instruction, EmitPcMode pcMode) -> EmitExecuteResult {
  (void)pcMode;
  switch(instruction >> 21 & 0x1f) {

  //MFC0 Rt,Rd
  case 0x00: {
    setupCallf();
    callf(&CPU::MFC0, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
  }

  //DMFC0 Rt,Rd
  case 0x01: {
    setupCallf();
    callf(&CPU::DMFC0, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
  }

  //INVALID
  case range2(0x02, 0x03): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    setupCallf();
    callf(&CPU::MTC0, mem(Rt), imm(Rdn));
    return EmitExecuteResult::MayFault;
  }

  //DMTC0 Rt,Rd
  case 0x05: {
    setupCallf();
    callf(&CPU::DMTC0, mem(Rt), imm(Rdn));
    return EmitExecuteResult::MayFault;
  }

  //INVALID
  case range10(0x06, 0x0f): {
    setupCallf();
    callf(&CPU::INVALID);
    return EmitExecuteResult::MayFault;
  }

  }

  switch(instruction & 0x3f) {

  //TLBR
  case 0x01: {
    setupCallf();
    callf(&CPU::TLBR);
    return EmitExecuteResult::MayFault;
  }

  //TLBWI
  case 0x02: {
    setupCallf();
    callf(&CPU::TLBWI);
    return EmitExecuteResult::MayFault;
  }

  //TLBWR
  case 0x06: {
    setupCallf();
    callf(&CPU::TLBWR);
    return EmitExecuteResult::MayFault;
  }

  //TLBP
  case 0x08: {
    setupCallf();
    callf(&CPU::TLBP);
    return EmitExecuteResult::MayFault;
  }

  //ERET
  case 0x18: {
    setupCallf();
    callf(&CPU::ERET);
    return EmitExecuteResult::MayFault;
  }

  //XDETECT
  case 0x20: {
    setupCallf();
    callf(&CPU::XDETECT, mem(XRd), imm(XCODE));
    return EmitExecuteResult::Linear;
  }

  //XLOG
  case 0x25: {
    setupCallf();
    callf(&CPU::XLOG, mem(XRd), mem(XRt), imm(XCODE));
    return EmitExecuteResult::Linear;
  }

  //XHEXDUMP
  case 0x27: {
    setupCallf();
    callf(&CPU::XHEXDUMP, mem(XRd), mem(XRt));
    return EmitExecuteResult::Linear;
  }

  //XPROF
  case 0x28: {
    setupCallf();
    callf(&CPU::XPROF, mem(XRd), imm(XCODE));
    return EmitExecuteResult::Linear;
  }

  //XPROFREAD
  case 0x29: {
    setupCallf();
    callf(&CPU::XPROFREAD, mem(XRd), mem(XRt));
    return EmitExecuteResult::Linear;
  }

  //XEXCEPTION
  case 0x2a: {
    setupCallf();
    callf(&CPU::XEXCEPTION, mem(XRt));
    return EmitExecuteResult::Linear;
  }

  //XIOCTL
  case 0x2c: {
    setupCallf();
    callf(&CPU::XIOCTL, imm(XCODE));
    return EmitExecuteResult::Linear;
  }

  }

  return EmitExecuteResult::Linear;
}
auto CPU::Recompiler::emitCOP2(u32 instruction) -> EmitExecuteResult {
  switch(instruction >> 21 & 0x1f) {

  //MFC2 Rt,Rd
  case 0x00: {
    setupCallf();
    callf(&CPU::MFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
  }

  //DMFC2 Rt,Rd
  case 0x01: {
    setupCallf();
    callf(&CPU::DMFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    setupCallf();
    callf(&CPU::CFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
  }

  //INVALID
  case 0x03: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return EmitExecuteResult::MayFault;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    setupCallf();
    callf(&CPU::MTC2, mem(Rt), imm(Rdn));
    return EmitExecuteResult::MayFault;
  }

  //DMTC2 Rt,Rd
  case 0x05: {
    setupCallf();
    callf(&CPU::DMTC2, mem(Rt), imm(Rdn));
    return EmitExecuteResult::MayFault;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    setupCallf();
    callf(&CPU::CTC2, mem(Rt), imm(Rdn));
    return EmitExecuteResult::MayFault;
  }

  //INVALID
  case range9(0x07, 0x0f): {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return EmitExecuteResult::MayFault;
  }

  }
  return EmitExecuteResult::Linear;
}

#undef callf

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
#undef XRtn
#undef XRdn
#undef XCODE
#undef Rd
#undef Rt
#undef Rt32
#undef Rs
#undef Rs32
#undef Lo
#undef Hi
#undef XRd
#undef XRt
#undef i16
#undef n16
#undef n26

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
