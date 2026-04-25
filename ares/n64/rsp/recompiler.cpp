/*
RSP Recompiler: Architecture and Optimization Notes
===================================================

Overview
--------
This file implements the Nintendo 64 RSP recompiler backend built on top of
nall::recompiler (SLJIT). The recompiler translates IMEM instructions into host
code blocks and executes them through RSP::Recompiler::Block::execute().

The design goal is to keep the hot path as close as possible to pure opcode
semantics, moving uncommon behaviors out of line and minimizing helper calls.
Correctness is preserved by matching interpreter-visible branch, delay-slot, and
clock behavior.

Block lifecycle
---------------
1) Block discovery
   - measure(): determines block size from a start PC until terminal condition.
   - decoder flags decide branch/end-block boundaries.

2) Block identity and reuse
   - hash(): hashes IMEM bytes for the measured range.
   - block(): combines IMEM hash + pipeline hash + start address.
   - context[] is a direct PC-indexed cache; blocks hashset handles dedup.
   - dirty mask invalidates overlapping cached blocks when IMEM changes.

3) Block emission and finalization
   - emit(): builds host code with beginFunction()/endFunction().
   - block->pipeline stores end-of-block pipeline state used by execute().

Execution model inside emit()
-----------------------------
The main emission loop models RSP scheduling semantics:
- decode current instruction to OpInfo (decoderEXECUTE()).
- begin pipeline window, issue first op, and emit host code via emitEXECUTE().
- opportunistically decode/issue a second opcode if canDualIssue() allows it.
- update constant-register tracking state with constRegs.track() per issued op.
- end pipeline window, update branch/state/PC bookkeeping, and exit if needed.

Instruction emission is split by decode family:
- emitEXECUTE(): top-level primary opcode dispatch.
- emitSPECIAL(), emitREGIMM(), emitSCC(), emitVU(), emitLWC2(), emitSWC2():
  specialized sub-decoders.

Delay slots and branch commit
-----------------------------
Branch handling is explicit:
- branch target/state are emitted directly into BranchReg(...) fields.
- delay-slot completion can call instructionBranchEpilogue() where needed.
- commit logic updates architectural PC and branch state at controlled points.

This keeps branch behavior deterministic while avoiding per-opcode generic
epilogue calls on the common non-branch path.

Out-of-line slow paths
----------------------
Rare paths are emitted after the main block body:
- slowPaths: deferred uncommon opcode cases (for example memory wraparound
  fallbacks that call interpreter helpers).
- haltSlowPaths: deferred exits when halted state must terminate execution.

Fast path emits a conditional jump to the slow-path section, then continues.
Each slow path jumps back to a resume label in the hot region after completion.
This reduces instruction cache pressure in the hot stream.

Key optimizations
-----------------
1) Deferred clock accounting (clock cache)
   - JIT-time: the emitter tracks pending cycles in deferredClocks instead of
     emitting per-opcode clock updates in the hot path.
   - Runtime applies pending cycles ("flushes them") to ThreadReg(clock) and
     PipelineReg(clocksTotal) by executing the emitted add instructions.
   - Flushes are emitted only at synchronization points (helper-call boundaries,
     block end, and slow-path transitions).
   - slowPathFlushedClocks prevents double-accounting when runtime control
     enters a slow path and later reaches another flush point.

2) Call-boundary synchronization only when needed
   - instructionMayCallf() identifies opcodes that execute C++ helpers.
   - JIT-time: a flush sequence is emitted only before helper calls that need
     an up-to-date clock value at runtime.

3) Selective halted checks
   - OpInfo::MayHalt limits halted-state exit checks to opcodes that can
     actually modify status.halted (for example BREAK or MTC0 to SP_STATUS).

4) R0 write suppression
   - JIT paths avoid emitting writes when destination is register 0.
   - C++ helper paths guard writeback so architectural R0 remains immutable.

5) Scalar DMEM fast paths + endian-aware ops
   - LB/LH/LW/LBU/LHU/LWU/SB/SH/SW are emitted directly in JIT fast paths.
   - Byte swap helpers are used where required by DMEM big-endian semantics.
   - Uncommon wraparound/alignment fallback is handled via deferred slow paths.

6) Dedicated DMEM base saved register
   - Host saved register sreg(3) holds dmem.data for the whole compiled block.
   - Scalar and vector DMEM fast paths address memory from this fixed base.
   - Because sreg(3) is callee-saved, helper calls do not require cache
     invalidation/reload logic.

7) Compile-time virtual PC usage
   - Many branch/link/fallthrough values are emitted as immediates derived from
     compile-time PC, reducing runtime state traffic in common paths.
   - This also removes the need to update architectural PC after every opcode;
     PC is committed only at explicit block/branch commit points.
   - Block hashing includes the block start address, so identical opcode bytes
     at different addresses cannot alias to the same compiled block by mistake.

8) Intra-block constant-register tracking
   - constRegs tracks which GPRs hold compile-time-known values while a block is
     emitted, propagating constants through a selected subset of IPU opcodes.
   - The tracker treats R0 as always constant zero and clears destinations when
     an opcode does not preserve constantness.
   - JIT address generation can fold base+offset immediates when Rs is known,
     reducing runtime arithmetic in scalar DMEM ops and vector fast paths.
   - When constantness is unknown or invalidated, emission falls back to regular
     runtime address computation without changing architectural behavior.

Maintenance guidelines
----------------------
- Preserve interpreter-visible behavior first (branching, delay slots, clocks).
- Keep uncommon logic in slow paths whenever possible.
- If adding new helper calls, ensure clock synchronization remains correct.
*/

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

auto RSP::Recompiler::measure(u12 address) -> u12 {
  u12 start = address;
  bool hasDelaySlot = 0;
  while(true) {
    u32 instruction = self.imem.read<Word>(address);
    OpInfo op = self.decoderEXECUTE(instruction);
    bool branched = op.branch();
    bool endBlock = op.endBlock();
    address += 4;
    if(hasDelaySlot || endBlock || address == start) break;
    hasDelaySlot = branched;
  }

  return address - start;
}

auto RSP::Recompiler::hash(u12 address, u12 size) -> u64 {
  u12 end = address + size;
  if(address < end) {
    return XXH3_64bits(self.imem.data + address, size);
  } else {
    return XXH3_64bits(self.imem.data + address, self.imem.size - address)
         ^ XXH3_64bits(self.imem.data, end);
  }
}

auto RSP::Recompiler::ConstRegs::track(u32 instruction, u32 pc) -> void {
  auto setKnownBinary = [&](u32 rd, u32 rs, u32 rt, auto&& op) -> void {
    if(!rd) return;
    if(!has(rs) || !has(rt)) return clear(rd);
    set(rd, op(get(rs), get(rt)));
  };
  auto setKnownShiftImmediate = [&](u32 rd, u32 rt, auto&& op) -> void {
    if(!rd) return;
    if(!has(rt)) return clear(rd);
    set(rd, op(get(rt)));
  };
  auto setKnownShiftVariable = [&](u32 rd, u32 rt, u32 rs, auto&& op) -> void {
    if(!rd) return;
    if(!has(rt) || !has(rs)) return clear(rd);
    set(rd, op(get(rt), get(rs) & 31));
  };
  auto setKnownImmediateBinary = [&](u32 rt, u32 rs, auto&& op) -> void {
    if(!rt) return;
    if(!rs) return set(rt, op(0));
    if(!has(rs)) return clear(rt);
    set(rt, op(get(rs)));
  };

  u32 opcode = instruction >> 26;
  u32 rs = instruction >> 21 & 31;
  u32 rt = instruction >> 16 & 31;
  u32 rd = instruction >> 11 & 31;
  u32 sa = instruction >> 6 & 31;
  s32 imm = s16(instruction);
  u32 n16 = u16(instruction);

  switch(opcode) {
  // SPECIAL
  case 0x00: {
    u32 function = instruction & 0x3f;
    switch(function) {
    // SLL
    case 0x00: return setKnownShiftImmediate(rd, rt, [&](u32 rtValue) { return rtValue << sa; });
    // SRL
    case 0x02: return setKnownShiftImmediate(rd, rt, [&](u32 rtValue) { return rtValue >> sa; });
    // SRA
    case 0x03: return setKnownShiftImmediate(rd, rt, [&](u32 rtValue) { return s32(rtValue) >> sa; });
    // SLLV
    case 0x04: return setKnownShiftVariable(rd, rt, rs, [&](u32 rtValue, u32 rsValue) { return rtValue << rsValue; });
    // SRLV
    case 0x06: return setKnownShiftVariable(rd, rt, rs, [&](u32 rtValue, u32 rsValue) { return rtValue >> rsValue; });
    // SRAV
    case 0x07: return setKnownShiftVariable(rd, rt, rs, [&](u32 rtValue, u32 rsValue) { return s32(rtValue) >> rsValue; });
    // JALR
    case 0x09: {
      if(rd) { set(rd, u32(u12(pc + 8))); return; }
    }
    // ADD / ADDU
    case 0x20:
    case 0x21: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return rsValue + rtValue; });
    // SUB / SUBU
    case 0x22:
    case 0x23: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return rsValue - rtValue; });
    // AND
    case 0x24: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return rsValue & rtValue; });
    // OR
    case 0x25: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return rsValue | rtValue; });
    // XOR
    case 0x26: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return rsValue ^ rtValue; });
    // NOR
    case 0x27: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return ~(rsValue | rtValue); });
    // SLT
    case 0x2a: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return u32(s32(rsValue) < s32(rtValue)); });
    // SLTU
    case 0x2b: return setKnownBinary(rd, rs, rt, [&](u32 rsValue, u32 rtValue) { return u32(rsValue < rtValue); });
    // JR / BREAK
    case 0x08:
    case 0x0d:
      return;
    default:
      if(rd) { clear(rd); return; }
    }
  }
  // REGIMM (BLTZAL / BGEZAL)
  case 0x01: {
    if(rt == 0x10 || rt == 0x11 || rt == 0x12 || rt == 0x13) { set(31, u32(u12(pc + 8))); return; }
  }
  // JAL
  case 0x03:
    set(31, u32(u12(pc + 8)));
    return;
  // ADDI / ADDIU
  case 0x08:
  case 0x09:
    return setKnownImmediateBinary(rt, rs, [&](u32 rsValue) { return rsValue + imm; });
  // SLTI
  case 0x0a:
    return setKnownImmediateBinary(rt, rs, [&](u32 rsValue) { return u32(s32(rsValue) < imm); });
  // SLTIU
  case 0x0b:
    return setKnownImmediateBinary(rt, rs, [&](u32 rsValue) { return u32(rsValue < u32(imm)); });
  // ANDI
  case 0x0c:
    return setKnownImmediateBinary(rt, rs, [&](u32 rsValue) { return rsValue & n16; });
  // ORI
  case 0x0d:
    return setKnownImmediateBinary(rt, rs, [&](u32 rsValue) { return rsValue | n16; });
  // XORI
  case 0x0e:
    return setKnownImmediateBinary(rt, rs, [&](u32 rsValue) { return rsValue ^ n16; });
  // LUI
  case 0x0f:
    if(rt) { set(rt, s32(n16 << 16)); return; }
  // COP0 / COP2
  case 0x10:
  case 0x12:
    if(rt) { clear(rt); return; }
  // LB / LH / LW / LBU / LHU / LWU
  case 0x20:
  case 0x21:
  case 0x23:
  case 0x24:
  case 0x25:
  case 0x27:
    if(rt) { clear(rt); return; }
  default:
    return;
  }
}

auto RSP::Recompiler::block(u12 address) -> Block* {
  auto contextIndex = [&](u12 address, u32 callInstructionPrologue) -> u32 {
    return callInstructionPrologue * 1024 + (address >> 2);
  };

  if(dirty) {
    for(u32 callInstructionPrologue : range(2)) {
      u12 pc = 0;
      for(u32 n : range(1024)) {
        auto& block = context[callInstructionPrologue * 1024 + n];
        if(block && (dirty & mask(pc, block->size)) != 0) {
          block = nullptr;
        }
        pc += 4;
      }
    }
    dirty = 0;
  }

  bool traceMode = self.debugger.tracer.instruction->enabled();
  bool callInstructionPrologue = traceMode;
  u32 index = contextIndex(address, callInstructionPrologue);
  if(auto block = context[index]) return block;

  auto size = measure(address);
  auto hashcode = hash(address, size);
  hashcode ^= self.pipeline.hash();
  hashcode ^= address;
  hashcode ^= callInstructionPrologue ? 0x6a09e667f3bcc909ull : 0;

  BlockHashPair pair;
  pair.hashcode = hashcode;
  if(auto result = blocks.find(pair)) {
    return context[index] = result->block;
  }

  auto block = emit(address, callInstructionPrologue);
  assert(block->size == size);
  memory::jitprotect(true);

  pair.block = block;
  if(auto result = blocks.insert(pair)) {
    return context[index] = result->block;
  }

  throw;  //should never occur
}

#define IpuReg(r) sreg(1), offsetof(IPU, r)
#define VuReg(r)  sreg(2), offsetof(VU, r)
#define DmemReg   sreg(3)
#define ThreadReg(x) mem(sreg(0), offsetof(RSP, x))
#define PipelineReg(x) mem(sreg(0), offsetof(RSP, pipeline) + offsetof(Pipeline, x))
#define BranchReg(x) mem(sreg(0), offsetof(RSP, branch) + offsetof(Branch, x))
#define StatusReg(x) mem(sreg(0), offsetof(RSP, status) + offsetof(Status, x))
#define RecompilerReg(x) mem(sreg(0), offsetof(RSP, recompiler) + offsetof(Recompiler, x))
#define R0        IpuReg(r[0])
#if defined(ARCHITECTURE_AMD64) || defined(ARCHITECTURE_ARM64)
  #define RSP_JIT_SUPPORTS_MISALIGNED_MEMORY_ACCESSES 1
#else
  #define RSP_JIT_SUPPORTS_MISALIGNED_MEMORY_ACCESSES 0
#endif

auto RSP::Recompiler::emit(u12 address, bool callInstructionPrologue) -> Block* {
  if(unlikely(allocator.available() < 128_KiB)) {
    print("RSP JIT: flushing all blocks\n");
    allocator.release();
    reset();
  }

  pipeline = self.pipeline;

  auto block = (Block*)allocator.acquire(sizeof(Block));
  beginFunction(3, 4);
  u32 deferredClocks = 0;
  mov32(RecompilerReg(slowPathFlushedClocks), imm(0));
  haltSlowPaths.clear();
  slowPaths.clear();
  constRegs.reset();
  mov64(DmemReg, mem(sreg(0), offsetof(RSP, dmem) + offsetof(Writable, data)));

  auto emitClockFlush = [&](u32 clocks) -> void {
    if(!clocks) return;
    add64(ThreadReg(clock), ThreadReg(clock), imm(clocks));
    add32(PipelineReg(clocksTotal), PipelineReg(clocksTotal), imm(clocks));
  };
  auto emitClockFlushRegister = [&](reg clocks) -> void {
    mov64_u32(reg(1), clocks);
    add64(ThreadReg(clock), ThreadReg(clock), reg(1));
    add32(PipelineReg(clocksTotal), PipelineReg(clocksTotal), clocks);
  };
  auto flushDeferredForCallf = [&]() -> void {
    if(!deferredClocks) return;
    sub32(reg(0), imm(deferredClocks), RecompilerReg(slowPathFlushedClocks));
    emitClockFlushRegister(reg(0));
    mov32(RecompilerReg(slowPathFlushedClocks), imm(0));
    deferredClocks = 0;
  };
  auto flushDeferredAtBlockEnd = [&]() -> void {
    if(!deferredClocks) return;
    sub32(reg(0), imm(deferredClocks), RecompilerReg(slowPathFlushedClocks));
    emitClockFlushRegister(reg(0));
    mov32(RecompilerReg(slowPathFlushedClocks), imm(0));
  };
  auto flushDeferredForSlowPath = [&](u32 clocks) -> void {
    sub32(reg(0), imm(clocks), RecompilerReg(slowPathFlushedClocks));
    emitClockFlushRegister(reg(0));
    mov32(RecompilerReg(slowPathFlushedClocks), imm(clocks));
  };
  auto enqueueHaltSlowPath = [&](sljit_jump* enter, u32 clocks) -> void {
    auto& slow = haltSlowPaths.emplace_back();
    slow.enter = enter;
    slow.clocks = clocks;
  };
  auto instructionMayCallf = [&](u32 instruction) -> bool {
    switch(instruction >> 26) {
    case 0x00: return (instruction & 0x3f) == 0x0d;  //BREAK
    case 0x10: return 1;  //SCC/XBUS
    case 0x32: return 1;  //LWC2
    case 0x3a: return 1;  //SWC2
    }
    return 0;
  };
  auto emitInstructionEpilogue = [&](u32 clocks, bool exit, bool delaySlot, bool commit, bool branched, u32 nextpc, bool checkHalted) -> void {
    if(delaySlot) {
      flushDeferredForCallf();
      emitClockFlush(clocks);
      callf(&RSP::instructionBranchEpilogue);
      if(exit) { testJumpEpilog(); return; }
    }

    if(callInstructionPrologue) emitClockFlush(clocks);
    else                        deferredClocks += clocks;

    if(commit) {
      if(!branched) mov32(BranchReg(state), imm(0));
      mov32(mem(IpuReg(pc)), imm(nextpc));
    }

    if(exit && checkHalted) {
      and32(reg(0), StatusReg(halted), imm(1));
      cmp32(reg(0), imm(0), set_z);
      auto rare = jump(flag_nz);
      enqueueHaltSlowPath(rare, deferredClocks);
    }
  };

  u12 start = address;
  bool delaySlot = 0;
  while(true) {
    u32 instruction = self.imem.read<Word>(address);
    if(instructionMayCallf(instruction)) {
      flushDeferredForCallf();
    }
    if(callInstructionPrologue) {
      callf(&RSP::instructionPrologue, imm(instruction));
    }
    if(delaySlot) mov32(BranchReg(nstate), imm(0));
    pipeline.begin();
    OpInfo op0 = self.decoderEXECUTE(instruction);
    bool branched = op0.branch();
    bool endBlock = op0.endBlock();
    bool checkHalted = op0.mayHalt();
    pipeline.issue(op0);
    emitEXECUTE(instruction, address, delaySlot, 0, deferredClocks);
    constRegs.track(instruction, address);

    if(!pipeline.singleIssue && !branched && !endBlock && u12(address + 4) != start) {
      u32 instruction = self.imem.read<Word>(address + 4);
      OpInfo op1 = self.decoderEXECUTE(instruction);

      if(RSP::canDualIssue(op0, op1)) {
        emitInstructionEpilogue(0, 0, delaySlot, callInstructionPrologue, 0, u32(u12(address + 4)), 0);
        if(instructionMayCallf(instruction)) {
          flushDeferredForCallf();
        }
        if(callInstructionPrologue) {
          callf(&RSP::instructionPrologue, imm(instruction));
        }
        checkHalted |= op1.mayHalt();
        address += 4;
        if(delaySlot) mov32(BranchReg(nstate), imm(0));
        pipeline.issue(op1);
        emitEXECUTE(instruction, address, delaySlot, 0, deferredClocks);
        branched = op1.branch();
        endBlock = op1.endBlock();
        constRegs.track(instruction, address);
      }
    }

    pipeline.end();
    bool terminal = delaySlot || endBlock || u12(address + 4) == start;
    bool commit = callInstructionPrologue || branched || terminal;
    emitInstructionEpilogue(pipeline.clocks, 1, delaySlot, commit, branched, u32(u12(address + 4)), checkHalted);
    address += 4;
    if(delaySlot || endBlock || address == start) break;
    delaySlot = branched;
  }
  flushDeferredAtBlockEnd();
  deferredClocks = 0;
  jumpEpilog();
  for(auto& slow : haltSlowPaths) {
    setLabel(slow.enter);
    flushDeferredForSlowPath(slow.clocks);
    mov32(RecompilerReg(slowPathFlushedClocks), imm(0));
    jumpEpilog();
  }
  for(auto& slow : slowPaths) {
    setLabel(slow.enter);
    flushDeferredForSlowPath(slow.clocks);
    emitEXECUTE(slow.instruction, slow.pc, slow.delaySlot, 1, 0);
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), slow.resume);
  }

  //reset clocks to zero every time block is executed
  pipeline.clocks = 0;

  memory::jitprotect(false);
  block->code = endFunction();
  block->size = address - start;
  block->pipeline = pipeline;

//print(hex(PC, 8L), " ", instructions, " ", size(), "\n");
  return block;
}

#define Sa  (instruction >>  6 & 31)
#define Rdn (instruction >> 11 & 31)
#define Rtn (instruction >> 16 & 31)
#define Rsn (instruction >> 21 & 31)
#define XRtn (instruction >> 15 & 31)
#define XRdn (instruction >> 20 & 31)
#define XCODE (instruction >> 6 & 511)
#define Vdn (instruction >>  6 & 31)
#define Vsn (instruction >> 11 & 31)
#define Vtn (instruction >> 16 & 31)
#define Rd  IpuReg(r[0]) + Rdn * sizeof(r32)
#define Rt  IpuReg(r[0]) + Rtn * sizeof(r32)
#define Rs  IpuReg(r[0]) + Rsn * sizeof(r32)
#define XRd IpuReg(r[0]) + XRdn * sizeof(r32)
#define XRt IpuReg(r[0]) + XRtn * sizeof(r32)
#define Vd  VuReg(r[0]) + Vdn * sizeof(r128)
#define Vs  VuReg(r[0]) + Vsn * sizeof(r128)
#define Vt  VuReg(r[0]) + Vtn * sizeof(r128)
#define i16 s16(instruction)
#define n16 u16(instruction)
#define n26 u32(instruction & 0x03ff'ffff)

#define callvu(name,...) \
  switch(E) { \
  case 0x0: callf(name<0x0>, __VA_ARGS__); break; \
  case 0x1: callf(name<0x1>, __VA_ARGS__); break; \
  case 0x2: callf(name<0x2>, __VA_ARGS__); break; \
  case 0x3: callf(name<0x3>, __VA_ARGS__); break; \
  case 0x4: callf(name<0x4>, __VA_ARGS__); break; \
  case 0x5: callf(name<0x5>, __VA_ARGS__); break; \
  case 0x6: callf(name<0x6>, __VA_ARGS__); break; \
  case 0x7: callf(name<0x7>, __VA_ARGS__); break; \
  case 0x8: callf(name<0x8>, __VA_ARGS__); break; \
  case 0x9: callf(name<0x9>, __VA_ARGS__); break; \
  case 0xa: callf(name<0xa>, __VA_ARGS__); break; \
  case 0xb: callf(name<0xb>, __VA_ARGS__); break; \
  case 0xc: callf(name<0xc>, __VA_ARGS__); break; \
  case 0xd: callf(name<0xd>, __VA_ARGS__); break; \
  case 0xe: callf(name<0xe>, __VA_ARGS__); break; \
  case 0xf: callf(name<0xf>, __VA_ARGS__); break; \
  }

auto RSP::Recompiler::emitEXECUTE(u32 instruction, u32 pc, bool delaySlot, bool emitSlowPath, u32 slowPathClocks) -> void {
  auto memReg2 = [&](const op_base& base, const op_base& index) -> op_base {
    return {SLJIT_MEM2(base.fst, index.fst), 0};
  };
  auto emitDmemAddress = [&]() -> void {
    if(!Rsn) {
      mov32(reg(1), imm(u32(u12(i16))));
    } else if(constRegs.has(Rsn)) {
      mov32(reg(1), imm(u32(u12(constRegs.get(Rsn) + i16))));
    } else {
      add32(reg(1), mem(Rs), imm(i16));
      and32(reg(1), reg(1), imm(0x0fff));
    }
  };
  auto emitDmemUnalignedJump = [&](u32 mask) -> sljit_jump* {
    and32(reg(0), reg(1), imm(mask));
    cmp32(reg(0), imm(0), set_z);
    return jump(flag_nz);
  };
  auto emitDmemHalfSlowPathJump = [&]() -> sljit_jump* {
    if constexpr(RSP_JIT_SUPPORTS_MISALIGNED_MEMORY_ACCESSES) {
      cmp32(reg(1), imm(0x0fff), set_z);
      return jump(flag_z);
    } else {
      return emitDmemUnalignedJump(1);
    }
  };
  auto emitDmemWordSlowPathJump = [&]() -> sljit_jump* {
    if constexpr(RSP_JIT_SUPPORTS_MISALIGNED_MEMORY_ACCESSES) {
      cmp32(reg(1), imm(0x0ffd), set_uge);
      return jump(flag_uge);
    } else {
      return emitDmemUnalignedJump(3);
    }
  };
  auto deferSlowPath = [&](sljit_jump* enter) -> void {
    auto& slow = slowPaths.emplace_back();
    slow.enter = enter;
    slow.resume = sljit_emit_label(compiler);
    slow.instruction = instruction;
    slow.pc = pc;
    slow.delaySlot = delaySlot;
    slow.clocks = slowPathClocks;
  };

  auto emitConditionalTake = [&](u32 flag, bool invert = 0) -> void {
    u32 target      = u32(u12(pc + 4 + s32(i16) * 4));
    u32 fallthrough = u32(u12(pc + 8));
    u32 selectMask  = fallthrough ^ target;
    mov32_f(reg(0), flag);
    if(invert) xor32(reg(0), reg(0), imm(1));
    sub32(reg(0), imm(0), reg(0));
    and32(reg(1), reg(0), imm(selectMask));
    xor32(reg(1), reg(1), imm(fallthrough));
    if(delaySlot) mov32(BranchReg(nextpc), reg(1));
    else          mov32(BranchReg(pc), reg(1));
    if(delaySlot) and32(BranchReg(nstate), reg(0), imm(Branch::DelaySlot | Branch::EndBlock));
    else          and32(BranchReg(state), reg(0), imm(Branch::DelaySlot | Branch::EndBlock));
  };
  auto emitKnownConditionalTake = [&](bool take) -> void {
    u32 target      = u32(u12(pc + 4 + s32(i16) * 4));
    u32 fallthrough = u32(u12(pc + 8));
    u32 destination = take ? target : fallthrough;
    u32 state       = take ? Branch::DelaySlot | Branch::EndBlock : 0;
    if(delaySlot) mov32(BranchReg(nextpc), imm(destination));
    else          mov32(BranchReg(pc), imm(destination));
    if(delaySlot) mov32(BranchReg(nstate), imm(state));
    else          mov32(BranchReg(state), imm(state));
  };

  switch(instruction >> 26) {

  //SPECIAL
  case 0x00: {
    emitSPECIAL(instruction, pc, delaySlot); return;
  }

  //REGIMM
  case 0x01: {
    emitREGIMM(instruction, pc, delaySlot); return;
  }

  //J n26
  case 0x02: {
    if(delaySlot) mov32(BranchReg(nextpc), imm(u32(u12(n26 << 2))));
    else          mov32(BranchReg(pc), imm(u32(u12(n26 << 2))));
    if(delaySlot) mov32(BranchReg(nstate), imm(Branch::DelaySlot | Branch::EndBlock));
    else          mov32(BranchReg(state), imm(Branch::DelaySlot | Branch::EndBlock));
    return;
  }

  //JAL n26
  case 0x03: {
    mov32(mem(IpuReg(r[31])), imm(u32(u12(pc + 8))));
    if(delaySlot) mov32(BranchReg(nextpc), imm(u32(u12(n26 << 2))));
    else          mov32(BranchReg(pc), imm(u32(u12(n26 << 2))));
    if(delaySlot) mov32(BranchReg(nstate), imm(Branch::DelaySlot | Branch::EndBlock));
    else          mov32(BranchReg(state), imm(Branch::DelaySlot | Branch::EndBlock));
    return;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    if(Rsn == Rtn) { emitKnownConditionalTake(1); return; }
    if(!Rsn) { cmp32(mem(Rt), imm(0), set_z), emitConditionalTake(flag_z); return; }
    if(!Rtn) { cmp32(mem(Rs), imm(0), set_z), emitConditionalTake(flag_z); return; }
    cmp32(mem(Rs), mem(Rt), set_z);
    emitConditionalTake(flag_z);
    return;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    if(Rsn == Rtn) { emitKnownConditionalTake(0); return; }
    if(!Rsn) { cmp32(mem(Rt), imm(0), set_z), emitConditionalTake(flag_z, 1); return; }
    if(!Rtn) { cmp32(mem(Rs), imm(0), set_z), emitConditionalTake(flag_z, 1); return; }
    cmp32(mem(Rs), mem(Rt), set_z);
    emitConditionalTake(flag_z, 1);
    return;
  }

  //BLEZ Rs,i16
  case 0x06: {
    if(!Rsn) { emitKnownConditionalTake(1); return; }
    cmp32(mem(Rs), imm(0), set_sgt);
    emitConditionalTake(flag_sgt, 1);
    return;
  }

  //BGTZ Rs,i16
  case 0x07: {
    if(!Rsn) { emitKnownConditionalTake(0); return; }
    cmp32(mem(Rs), imm(0), set_sgt);
    emitConditionalTake(flag_sgt);
    return;
  }

  //ADDIU Rt,Rs,i16
  case range2(0x08, 0x09): {
    if(!Rtn) return;
    if(!Rsn) { mov32(mem(Rt), imm(i16)); return; }
    add32(mem(Rt), mem(Rs), imm(i16));
    return;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    if(!Rtn) return;
    if(!Rsn) { mov32(mem(Rt), imm(i16 > 0)); return; }
    cmp32(mem(Rs), imm(i16), set_slt);
    mov32_f(mem(Rt), flag_slt);
    return;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    if(!Rtn) return;
    if(!Rsn) { mov32(mem(Rt), imm(i16 != 0)); return; }
    cmp32(mem(Rs), imm(i16), set_ult);
    mov32_f(mem(Rt), flag_ult);
    return;
  }

  //ANDI Rt,Rs,n16
  case 0x0c: {
    if(!Rtn) return;
    if(!Rsn) { mov32(mem(Rt), imm(0)); return; }
    and32(mem(Rt), mem(Rs), imm(n16));
    return;
  }

  //ORI Rt,Rs,n16
  case 0x0d: {
    if(!Rtn) return;
    if(!Rsn) { mov32(mem(Rt), imm(n16)); return; }
    or32(mem(Rt), mem(Rs), imm(n16));
    return;
  }

  //XORI Rt,Rs,n16
  case 0x0e: {
    if(!Rtn) return;
    if(!Rsn) { mov32(mem(Rt), imm(n16)); return; }
    xor32(mem(Rt), mem(Rs), imm(n16));
    return;
  }

  //LUI Rt,n16
  case 0x0f: {
    if(!Rtn) return;
    mov32(mem(Rt), imm(s32(n16 << 16)));
    return;
  }

  //SCC
  case 0x10: {
    emitSCC(instruction); return;
  }

  //INVALID
  case 0x11: {
    return;
  }

  //VPU
  case 0x12: {
    emitVU(instruction); return;
  }

  //INVALID
  case range13(0x13, 0x1f): {
    return;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    if(!Rtn) return;
    emitDmemAddress();
    mov32_s8(reg(3), memReg2(DmemReg, reg(1)));
    mov32(mem(Rt), reg(3));
    return;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    if(!Rtn) return;
    if(emitSlowPath) {
      callf(&RSP::LH, mem(Rt), mem(Rs), imm(i16));
      return;
    }
    emitDmemAddress();
    auto rare = emitDmemHalfSlowPathJump();
    mov32_u16(reg(3), memReg2(DmemReg, reg(1)));
    rev32_s16(reg(3), reg(3));
    mov32(mem(Rt), reg(3));
    deferSlowPath(rare);
    return;
  }

  //INVALID
  case 0x22: {
    return;
  }

  //LW Rt,Rs,i16
  case 0x23: {
    if(!Rtn) return;
    if(emitSlowPath) {
      callf(&RSP::LW, mem(Rt), mem(Rs), imm(i16));
      return;
    }
    emitDmemAddress();
    auto rare = emitDmemWordSlowPathJump();
    mov32(reg(3), memReg2(DmemReg, reg(1)));
    rev32(reg(3), reg(3));
    mov32(mem(Rt), reg(3));
    deferSlowPath(rare);
    return;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    if(!Rtn) return;
    emitDmemAddress();
    mov32_u8(reg(3), memReg2(DmemReg, reg(1)));
    mov32(mem(Rt), reg(3));
    return;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    if(!Rtn) return;
    if(emitSlowPath) {
      callf(&RSP::LHU, mem(Rt), mem(Rs), imm(i16));
      return;
    }
    emitDmemAddress();
    auto rare = emitDmemHalfSlowPathJump();
    mov32_u16(reg(3), memReg2(DmemReg, reg(1)));
    rev32_u16(reg(3), reg(3));
    mov32(mem(Rt), reg(3));
    deferSlowPath(rare);
    return;
  }

  //INVALID
  case 0x26: {
    return;
  }

  //LWU Rt,Rs,i16
  case 0x27: {
    if(!Rtn) return;
    if(emitSlowPath) {
      callf(&RSP::LWU, mem(Rt), mem(Rs), imm(i16));
      return;
    }
    emitDmemAddress();
    auto rare = emitDmemWordSlowPathJump();
    mov32(reg(3), memReg2(DmemReg, reg(1)));
    rev32(reg(3), reg(3));
    mov32(mem(Rt), reg(3));
    deferSlowPath(rare);
    return;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    emitDmemAddress();
    if(!Rtn) mov32(reg(3), imm(0));
    else     mov32(reg(3), mem(Rt));
    mov64_u8(memReg2(DmemReg, reg(1)), reg(3));
    return;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    if(emitSlowPath) {
      callf(&RSP::SH, mem(Rt), mem(Rs), imm(i16));
      return;
    }
    emitDmemAddress();
    auto rare = emitDmemHalfSlowPathJump();
    if(!Rtn) mov32(reg(3), imm(0));
    else     mov32(reg(3), mem(Rt));
    rev32_u16(reg(3), reg(3));
    mov64_u16(memReg2(DmemReg, reg(1)), reg(3));
    deferSlowPath(rare);
    return;
  }

  //INVALID
  case 0x2a: {
    return;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    if(emitSlowPath) {
      callf(&RSP::SW, mem(Rt), mem(Rs), imm(i16));
      return;
    }
    emitDmemAddress();
    auto rare = emitDmemWordSlowPathJump();
    if(!Rtn) mov32(reg(3), imm(0));
    else     mov32(reg(3), mem(Rt));
    rev32(reg(3), reg(3));
    mov32(memReg2(DmemReg, reg(1)), reg(3));
    deferSlowPath(rare);
    return;
  }

  //INVALID
  case range6(0x2c, 0x31): {
    return;
  }

  //LWC2
  case 0x32: {
    emitLWC2(instruction, pc, delaySlot, emitSlowPath, slowPathClocks); return;
  }

  //INVALID
  case range7(0x33, 0x39): {
    return;
  }

  //SWC2
  case 0x3a: {
    emitSWC2(instruction, pc, delaySlot, emitSlowPath, slowPathClocks); return;
  }

  //INVALID
  case range5(0x3b, 0x3f): {
    return;
  }

  }

  return;
}

auto RSP::Recompiler::emitSPECIAL(u32 instruction, u32 pc, bool delaySlot) -> void {
  switch(instruction & 0x3f) {

  //SLL Rd,Rt,Sa
  case 0x00: {
    if(!Rdn) return;
    if(!Rtn) { mov32(mem(Rd), imm(0)); return; }
    shl32(mem(Rd), mem(Rt), imm(Sa));
    return;
  }

  //INVALID
  case 0x01: {
    if(!Rdn) return;
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    if(!Rdn) return;
    if(!Rtn) { mov32(mem(Rd), imm(0)); return; }
    lshr32(mem(Rd), mem(Rt), imm(Sa));
    return;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    if(!Rdn) return;
    if(!Rtn) { mov32(mem(Rd), imm(0)); return; }
    ashr32(mem(Rd), mem(Rt), imm(Sa));
    return;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    if(!Rdn) return;
    if(!Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { mov32(mem(Rd), mem(Rt)); return; }
    mshl32(mem(Rd), mem(Rt), mem(Rs));
    return;
  }

  //INVALID
  case 0x05: {
    if(!Rdn) return;
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return;
  }

  //SRLV Rd,Rt,Rs
  case 0x06: {
    if(!Rdn) return;
    if(!Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { mov32(mem(Rd), mem(Rt)); return; }
    mlshr32(mem(Rd), mem(Rt), mem(Rs));
    return;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    if(!Rdn) return;
    if(!Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { mov32(mem(Rd), mem(Rt)); return; }
    mashr32(mem(Rd), mem(Rt), mem(Rs));
    return;
  }

  //JR Rs
  case 0x08: {
    if(!Rsn)      mov32(reg(0), imm(0));
    else          and32(reg(0), mem(Rs), imm(0x0fff));
    if(delaySlot) mov32(BranchReg(nextpc), reg(0));
    else          mov32(BranchReg(pc), reg(0));
    if(delaySlot) mov32(BranchReg(nstate), imm(Branch::DelaySlot | Branch::EndBlock));
    else          mov32(BranchReg(state), imm(Branch::DelaySlot | Branch::EndBlock));
    return;
  }

  //JALR Rd,Rs
  case 0x09: {
    if(!Rsn)      mov32(reg(0), imm(0));
    else          and32(reg(0), mem(Rs), imm(0x0fff));
    if(Rdn)       mov32(mem(Rd), imm(u32(u12(pc + 8))));
    if(delaySlot) mov32(BranchReg(nextpc), reg(0));
    else          mov32(BranchReg(pc), reg(0));
    if(delaySlot) mov32(BranchReg(nstate), imm(Branch::DelaySlot | Branch::EndBlock));
    else          mov32(BranchReg(state), imm(Branch::DelaySlot | Branch::EndBlock));
    return;
  }

  //INVALID
  case range3(0x0a, 0x0c): {
    if(!Rdn) return;
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return;
  }

  //BREAK
  case 0x0d: {
    callf(&RSP::BREAK);
    if(delaySlot) mov32(BranchReg(nstate), imm(0));
    else          mov32(BranchReg(state), imm(0));
    return;
  }

  //INVALID
  case range18(0x0e, 0x1f): {
    if(!Rdn) return;
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return;
  }

  //ADDU Rd,Rs,Rt
  case range2(0x20, 0x21): {
    if(!Rdn) return;
    if(!Rsn && !Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { mov32(mem(Rd), mem(Rt)); return; }
    if(!Rtn) { mov32(mem(Rd), mem(Rs)); return; }
    add32(mem(Rd), mem(Rs), mem(Rt));
    return;
  }

  //SUBU Rd,Rs,Rt
  case range2(0x22, 0x23): {
    if(!Rdn) return;
    if(!Rsn && !Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { sub32(mem(Rd), imm(0), mem(Rt)); return; }
    if(!Rtn) { mov32(mem(Rd), mem(Rs)); return; }
    sub32(mem(Rd), mem(Rs), mem(Rt));
    return;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    if(!Rdn) return;
    if(!Rsn || !Rtn) { mov32(mem(Rd), imm(0)); return; }
    and32(mem(Rd), mem(Rs), mem(Rt));
    return;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    if(!Rdn) return;
    if(!Rsn && !Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { mov32(mem(Rd), mem(Rt)); return; }
    if(!Rtn) { mov32(mem(Rd), mem(Rs)); return; }
    or32(mem(Rd), mem(Rs), mem(Rt));
    return;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    if(!Rdn) return;
    if(!Rsn && !Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { mov32(mem(Rd), mem(Rt)); return; }
    if(!Rtn) { mov32(mem(Rd), mem(Rs)); return; }
    xor32(mem(Rd), mem(Rs), mem(Rt));
    return;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    if(!Rdn) return;
    if(!Rsn && !Rtn) { mov32(mem(Rd), imm(-1)); return; }
    if(!Rsn) { xor32(mem(Rd), mem(Rt), imm(-1)); return; }
    if(!Rtn) { xor32(mem(Rd), mem(Rs), imm(-1)); return; }
    or32(reg(0), mem(Rs), mem(Rt));
    xor32(reg(0), reg(0), imm(-1));
    mov32(mem(Rd), reg(0));
    return;
  }

  //INVALID
  case range2(0x28, 0x29): {
    if(!Rdn) return;
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    if(!Rdn) return;
    if(!Rsn && !Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { cmp32(mem(Rt), imm(0), set_sgt), mov32_f(mem(Rd), flag_sgt); return; }
    if(!Rtn) { cmp32(mem(Rs), imm(0), set_slt), mov32_f(mem(Rd), flag_slt); return; }
    cmp32(mem(Rs), mem(Rt), set_slt);
    mov32_f(mem(Rd), flag_slt);
    return;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    if(!Rdn) return;
    if(!Rtn) { mov32(mem(Rd), imm(0)); return; }
    if(!Rsn) { cmp32(mem(Rt), imm(0), set_ugt), mov32_f(mem(Rd), flag_ugt); return; }
    cmp32(mem(Rs), mem(Rt), set_ult);
    mov32_f(mem(Rd), flag_ult);
    return;
  }

  //INVALID
  case range20(0x2c, 0x3f): {
    if(!Rdn) return;
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return;
  }
  }

  return;
}

auto RSP::Recompiler::emitREGIMM(u32 instruction, u32 pc, bool delaySlot) -> void {
  auto emitConditionalTake = [&](u32 flag) -> void {
    u32 target      = u32(u12(pc + 4 + s32(i16) * 4));
    u32 fallthrough = u32(u12(pc + 8));
    u32 selectMask  = fallthrough ^ target;
    mov32_f(reg(0), flag);
    sub32(reg(0), imm(0), reg(0));
    and32(reg(1), reg(0), imm(selectMask));
    xor32(reg(1), reg(1), imm(fallthrough));
    if(delaySlot) mov32(BranchReg(nextpc), reg(1));
    else          mov32(BranchReg(pc), reg(1));
    if(delaySlot) and32(BranchReg(nstate), reg(0), imm(Branch::DelaySlot | Branch::EndBlock));
    else          and32(BranchReg(state), reg(0), imm(Branch::DelaySlot | Branch::EndBlock));
  };
  auto emitKnownConditionalTake = [&](bool take) -> void {
    u32 target      = u32(u12(pc + 4 + s32(i16) * 4));
    u32 fallthrough = u32(u12(pc + 8));
    u32 destination = take ? target : fallthrough;
    u32 state       = take ? Branch::DelaySlot | Branch::EndBlock : 0;
    if(delaySlot) mov32(BranchReg(nextpc), imm(destination));
    else          mov32(BranchReg(pc), imm(destination));
    if(delaySlot) mov32(BranchReg(nstate), imm(state));
    else          mov32(BranchReg(state), imm(state));
  };

  switch(instruction >> 16 & 0x1f) {

  //BLTZ Rs,i16
  case 0x00: {
    if(!Rsn) { emitKnownConditionalTake(0); return; }
    cmp32(mem(Rs), imm(0), set_slt);
    emitConditionalTake(flag_slt);
    return;
  }

  //BGEZ Rs,i16
  case 0x01: {
    if(!Rsn) { emitKnownConditionalTake(1); return; }
    cmp32(mem(Rs), imm(0), set_sge);
    emitConditionalTake(flag_sge);
    return;
  }

  //INVALID
  case range14(0x02, 0x0f): {
    return;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    mov32(reg(3), imm(u32(u12(pc + 8))));
    if(!Rsn) { emitKnownConditionalTake(0), mov32(mem(IpuReg(r[31])), reg(3)); return; }
    cmp32(mem(Rs), imm(0), set_slt);
    emitConditionalTake(flag_slt);
    mov32(mem(IpuReg(r[31])), reg(3));
    return;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    mov32(reg(3), imm(u32(u12(pc + 8))));
    if(!Rsn) { emitKnownConditionalTake(1), mov32(mem(IpuReg(r[31])), reg(3)); return; }
    cmp32(mem(Rs), imm(0), set_sge);
    emitConditionalTake(flag_sge);
    mov32(mem(IpuReg(r[31])), reg(3));
    return;
  }

  //INVALID
  case range14(0x12, 0x1f): {
    return;
  }

  }

  return;
}

auto RSP::Recompiler::emitSCC(u32 instruction) -> void {
  switch(instruction >> 21 & 0x1f) {

  //MFC0 Rt,Rd
  case 0x00: {
    callf(&RSP::MFC0, mem(Rt), imm(Rdn));
    return;
  }

  //INVALID
  case range3(0x01, 0x03): {
    return;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    callf(&RSP::MTC0, mem(Rt), imm(Rdn));
    return;
  }

  //INVALID
  case range11(0x05, 0x0f): {
    return;
  }

  }

  switch(instruction & 0x3f) {

  //XDETECT
  case 0x20: {
    callf(&RSP::XDETECT, mem(XRd), imm(XCODE));
    return;
  }

  //XTRACE-START
  case 0x23: {
    callf(&RSP::XTRACESTART, imm(XCODE));
    return;
  }

  //XTRACE-STOP
  case 0x24: {
    callf(&RSP::XTRACESTOP);
    return;
  }

  //XLOG
  case 0x25: {
    callf(&RSP::XLOG, mem(XRd), mem(XRt), imm(XCODE));
    return;
  }

  //XLOGREGS
  case 0x26: {
    callf(&RSP::XLOGREGS, mem(XRd), imm(XCODE));
    return;
  }

  //XHEXDUMP
  case 0x27: {
    callf(&RSP::XHEXDUMP, mem(XRd), mem(XRt), imm(XCODE));
    return;
  }

  //XPROF
  case 0x28: {
    callf(&RSP::XPROF, mem(XRd), imm(XCODE));
    return;
  }

  //XPROFREAD
  case 0x29: {
    callf(&RSP::XPROFREAD, mem(XRd), mem(XRt));
    return;
  }

  //XEXCEPTION
  case 0x2a: {
    callf(&RSP::XEXCEPTION, mem(XRt));
    return;
  }

  //XIOCTL
  case 0x2c: {
    callf(&RSP::XIOCTL, imm(XCODE));
    return;
  }

  }

  return;
}

auto RSP::Recompiler::emitVU(u32 instruction) -> void {
  #define E (instruction >> 7 & 15)
  switch(instruction >> 21 & 0x1f) {

  //MFC2 Rt,Vs(e)
  case 0x00: {
    callvu(&RSP::MFC2, mem(Rt), mem(Vs));
    return;
  }

  //INVALID
  case 0x01: {
    return;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    callf(&RSP::CFC2, mem(Rt), imm(Rdn));
    return;
  }

  //INVALID
  case 0x03: {
    return;
  }

  //MTC2 Rt,Vs(e)
  case 0x04: {
    callvu(&RSP::MTC2, mem(Rt), mem(Vs));
    return;
  }

  //INVALID
  case 0x05: {
    return;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    callf(&RSP::CTC2, mem(Rt), imm(Rdn));
    return;
  }

  //INVALID
  case range9(0x07, 0x0f): {
    return;
  }

  }
  #undef E

  #define E  (instruction >> 21 & 15)
  #define DE (instruction >> 11 &  7)
  switch(instruction & 0x3f) {

  //VMULF Vd,Vs,Vt(e)
  case 0x00: {
    callvu(&RSP::VMULF, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMULU Vd,Vs,Vt(e)
  case 0x01: {
    callvu(&RSP::VMULU, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VRNDP Vd,Vs,Vt(e)
  case 0x02: {
    callvu(&RSP::VRNDP, mem(Vd), imm(Vsn), mem(Vt));
    return;
  }

  //VMULQ Vd,Vs,Vt(e)
  case 0x03: {
    callvu(&RSP::VMULQ, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMUDL Vd,Vs,Vt(e)
  case 0x04: {
    callvu(&RSP::VMUDL, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMUDM Vd,Vs,Vt(e)
  case 0x05: {
    callvu(&RSP::VMUDM, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMUDN Vd,Vs,Vt(e)
  case 0x06: {
    callvu(&RSP::VMUDN, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMUDH Vd,Vs,Vt(e)
  case 0x07: {
    callvu(&RSP::VMUDH, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMACF Vd,Vs,Vt(e)
  case 0x08: {
    callvu(&RSP::VMACF, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMACU Vd,Vs,Vt(e)
  case 0x09: {
    callvu(&RSP::VMACU, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VRNDN Vd,Vs,Vt(e)
  case 0x0a: {
    callvu(&RSP::VRNDN, mem(Vd), imm(Vsn), mem(Vt));
    return;
  }

  //VMACQ Vd
  case 0x0b: {
    callf(&RSP::VMACQ, mem(Vd));
    return;
  }

  //VMADL Vd,Vs,Vt(e)
  case 0x0c: {
    callvu(&RSP::VMADL, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMADM Vd,Vs,Vt(e)
  case 0x0d: {
    callvu(&RSP::VMADM, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMADN Vd,Vs,Vt(e)
  case 0x0e: {
    callvu(&RSP::VMADN, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMADH Vd,Vs,Vt(e)
  case 0x0f: {
    callvu(&RSP::VMADH, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VADD Vd,Vs,Vt(e)
  case 0x10: {
    callvu(&RSP::VADD, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VSUB Vd,Vs,Vt(e)
  case 0x11: {
    callvu(&RSP::VSUB, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VSUT (broken)
  case 0x12: {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VABS Vd,Vs,Vt(e)
  case 0x13: {
    callvu(&RSP::VABS, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VADDC Vd,Vs,Vt(e)
  case 0x14: {
    callvu(&RSP::VADDC, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VSUBC Vd,Vs,Vt(e)
  case 0x15: {
    callvu(&RSP::VSUBC, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //Broken opcodes: VADDB, VSUBB, VACCB, VSUCB, VSAD, VSAC, VSUM
  case range7(0x16, 0x1c): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VSAR Vd,Vs,E
  case 0x1d: {
    callvu(&RSP::VSAR, mem(Vd), mem(Vs));
    return;
  }

  //Invalid opcodes
  case range2(0x1e, 0x1f): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VLT Vd,Vs,Vt(e)
  case 0x20: {
    callvu(&RSP::VLT, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VEQ Vd,Vs,Vt(e)
  case 0x21: {
    callvu(&RSP::VEQ, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VNE Vd,Vs,Vt(e)
  case 0x22: {
    callvu(&RSP::VNE, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VGE Vd,Vs,Vt(e)
  case 0x23: {
    callvu(&RSP::VGE, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VCL Vd,Vs,Vt(e)
  case 0x24: {
    callvu(&RSP::VCL, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VCH Vd,Vs,Vt(e)
  case 0x25: {
    callvu(&RSP::VCH, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VCR Vd,Vs,Vt(e)
  case 0x26: {
    callvu(&RSP::VCR, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VMRG Vd,Vs,Vt(e)
  case 0x27: {
    callvu(&RSP::VMRG, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VAND Vd,Vs,Vt(e)
  case 0x28: {
    callvu(&RSP::VAND, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VNAND Vd,Vs,Vt(e)
  case 0x29: {
    callvu(&RSP::VNAND, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VOR Vd,Vs,Vt(e)
  case 0x2a: {
    callvu(&RSP::VOR, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VNOR Vd,Vs,Vt(e)
  case 0x2b: {
    callvu(&RSP::VNOR, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VXOR Vd,Vs,Vt(e)
  case 0x2c: {
    callvu(&RSP::VXOR, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VNXOR Vd,Vs,Vt(e)
  case 0x2d: {
    callvu(&RSP::VNXOR, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //INVALID
  case range2(0x2e, 0x2f): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //VCRP Vd(de),Vt(e)
  case 0x30: {
    callvu(&RSP::VRCP, mem(Vd), imm(DE), mem(Vt));
    return;
  }

  //VRCPL Vd(de),Vt(e)
  case 0x31: {
    callvu(&RSP::VRCPL, mem(Vd), imm(DE), mem(Vt));
    return;
  }

  //VRCPH Vd(de),Vt(e)
  case 0x32: {
    callvu(&RSP::VRCPH, mem(Vd), imm(DE), mem(Vt));
    return;
  }

  //VMOV Vd(de),Vt(e)
  case 0x33: {
    callvu(&RSP::VMOV, mem(Vd), imm(DE), mem(Vt));
    return;
  }

  //VRSQ Vd(de),Vt(e)
  case 0x34: {
    callvu(&RSP::VRSQ, mem(Vd), imm(DE), mem(Vt));
    return;
  }

  //VRSQL Vd(de),Vt(e)
  case 0x35: {
    callvu(&RSP::VRSQL, mem(Vd), imm(DE), mem(Vt));
    return;
  }

  //VRSQH Vd(de),Vt(e)
  case 0x36: {
    callvu(&RSP::VRSQH, mem(Vd), imm(DE), mem(Vt));
    return;
  }

  //VNOP
  case 0x37: {
    callf(&RSP::VNOP);
    return;
  }
//Broken opcodes: VEXTT, VEXTQ, VEXTN
  case range3(0x38, 0x3a): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return;        
  }

  //INVALID
  case 0x3b: {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return;
  }

  //Broken opcodes: VINST, VINSQ, VINSN
  case range3(0x3c, 0x3e): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return;        
  }

  //VNULL
  case 0x3f: {
    callf(&RSP::VNOP);
    return;
  }

  }
  #undef E
  #undef DE

  return;
}

alignas(16) extern u8 const simdLHVSource[16][8];
alignas(16) extern u8 const simdLFVData[16][32];
alignas(16) extern u8 const simdLRVSize[16][16];
alignas(16) extern u8 const simdSFVSource[16][4];
alignas(16) extern u8 const simdSRVStart[16][16];
alignas(16) extern u8 const simdSUVSource[16][8];
alignas(16) extern u8 const simdSWVSource[16][8];
alignas(16) extern u8 const simdSHVSource[16][32];

auto RSP::Recompiler::emitLWC2(u32 instruction, u32 pc, bool delaySlot, bool emitSlowPath, u32 slowPathClocks) -> void {
  auto memReg2 = [&](const op_base& base, const op_base& index) -> op_base {
    return {SLJIT_MEM2(base.fst, index.fst), 0};
  };
  auto deferSlowPath = [&](sljit_jump* enter) -> void {
    auto& slow = slowPaths.emplace_back();
    slow.enter = enter;
    slow.resume = sljit_emit_label(compiler);
    slow.instruction = instruction;
    slow.pc = pc;
    slow.delaySlot = delaySlot;
    slow.clocks = slowPathClocks;
  };
  #define E  (instruction >> 7 & 15)
  #define i7 (s8(instruction << 1) >> 1)
  switch(instruction >> 11 & 0x1f) {

  //LBV Vt(e),Rs,i7
  case 0x00: {
    if(constRegs.has(Rsn)) mov32(reg(1), imm(u32(u12(constRegs.get(Rsn) + i7))));
    else                   add32(reg(1), mem(Rs), imm(i7)), and32(reg(1), reg(1), imm(0x0fff));
    mov32_u8(reg(0), memReg2(DmemReg, reg(1)));
    mov64_u8(mem(sreg(2), offsetof(VU, r[0]) + Vtn * sizeof(r128) + (15 - E)), reg(0));
    return;
  }

  //LSV Vt(e),Rs,i7
  case 0x01: {
    if(emitSlowPath) {
      callvu(&RSP::LSV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    u32 size = 16 - E;
    if(size > 2) size = 2;
    auto vectorBase = offsetof(VU, r[0]) + Vtn * sizeof(r128);
    u32 addr = 0;
    if(constRegs.has(Rsn)) {
      addr = u32(u12(constRegs.get(Rsn) + i7 * 2));
      mov32(reg(1), imm(addr));
    } else {
      add32(reg(1), mem(Rs), imm(i7 * 2));
      and32(reg(1), reg(1), imm(0x0fff));
    }
    sljit_jump* slow = nullptr;
    if(size > 1) {
      u32 wrapLimit = 0x1000 - size;
      if(constRegs.has(Rsn)) {
        if(addr > wrapLimit) {
          callvu(&RSP::LSV, mem(Vt), mem(Rs), imm(i7));
          return;
        }
      } else {
        cmp32(reg(1), imm(wrapLimit), set_ugt);
        slow = jump(flag_ugt);
      }
    }
    if(size == 2) {
      mov32_u16(reg(0), memReg2(DmemReg, reg(1)));
      rev32_u16(reg(0), reg(0));
      mov64_u16(mem(sreg(2), vectorBase + (14 - E)), reg(0));
    } else {
      mov32_u8(reg(0), memReg2(DmemReg, reg(1)));
      mov64_u8(mem(sreg(2), vectorBase + (15 - E)), reg(0));
    }
    if(slow) { deferSlowPath(slow); return; }
    return;
  }

  //LLV Vt(e),Rs,i7
  case 0x02: {
    if(emitSlowPath) {
      callvu(&RSP::LLV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    u32 size = 16 - E;
    if(size > 4) size = 4;
    auto vectorBase = offsetof(VU, r[0]) + Vtn * sizeof(r128);
    u32 addr = 0;
    if(constRegs.has(Rsn)) {
      addr = u32(u12(constRegs.get(Rsn) + i7 * 4));
      mov32(reg(1), imm(addr));
    } else {
      add32(reg(1), mem(Rs), imm(i7 * 4));
      and32(reg(1), reg(1), imm(0x0fff));
    }
    sljit_jump* slow = nullptr;
    if(size > 1) {
      u32 wrapLimit = 0x1000 - size;
      if(constRegs.has(Rsn)) {
        if(addr > wrapLimit) {
          callvu(&RSP::LLV, mem(Vt), mem(Rs), imm(i7));
          return;
        }
      } else {
        cmp32(reg(1), imm(wrapLimit), set_ugt);
        slow = jump(flag_ugt);
      }
    }
    if(size == 4) {
      mov32(reg(0), memReg2(DmemReg, reg(1)));
      rev32(reg(0), reg(0));
      mov32(mem(sreg(2), vectorBase + (12 - E)), reg(0));
    } else if(size == 2) {
      mov32_u16(reg(0), memReg2(DmemReg, reg(1)));
      rev32_u16(reg(0), reg(0));
      mov64_u16(mem(sreg(2), vectorBase + (14 - E)), reg(0));
    } else {
      mov32_u8(reg(0), memReg2(DmemReg, reg(1)));
      mov64_u8(mem(sreg(2), vectorBase + (15 - E)), reg(0));
      if(size == 3) {
        add32(reg(1), reg(1), imm(1));
        mov32_u8(reg(0), memReg2(DmemReg, reg(1)));
        mov64_u8(mem(sreg(2), vectorBase + (15 - ((E + 1) & 15))), reg(0));
        add32(reg(1), reg(1), imm(1));
        mov32_u8(reg(0), memReg2(DmemReg, reg(1)));
        mov64_u8(mem(sreg(2), vectorBase + (15 - ((E + 2) & 15))), reg(0));
      }
    }
    if(slow) { deferSlowPath(slow); return; }
    return;
  }

  //LDV Vt(e),Rs,i7
  case 0x03: {
    if(emitSlowPath) {
      callvu(&RSP::LDV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    u32 size = 16 - E;
    if(size > 8) size = 8;
    if(size != 8 && size != 4 && size != 2 && size != 1) {
      callvu(&RSP::LDV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    auto vectorBase = offsetof(VU, r[0]) + Vtn * sizeof(r128);
    u32 addr = 0;
    if(constRegs.has(Rsn)) {
      addr = u32(u12(constRegs.get(Rsn) + i7 * 8));
      mov32(reg(1), imm(addr));
    } else {
      add32(reg(1), mem(Rs), imm(i7 * 8));
      and32(reg(1), reg(1), imm(0x0fff));
    }
    sljit_jump* slow = nullptr;
    if(size > 1) {
      u32 wrapLimit = 0x1000 - size;
      if(constRegs.has(Rsn)) {
        if(addr > wrapLimit) {
          callvu(&RSP::LDV, mem(Vt), mem(Rs), imm(i7));
          return;
        }
      } else {
        cmp32(reg(1), imm(wrapLimit), set_ugt);
        slow = jump(flag_ugt);
      }
    }
    if(size == 8) {
      mov32(reg(0), memReg2(DmemReg, reg(1)));
      rev32(reg(0), reg(0));
      mov32(mem(sreg(2), vectorBase + (12 - E)), reg(0));
      add32(reg(1), reg(1), imm(4));
      mov32(reg(0), memReg2(DmemReg, reg(1)));
      rev32(reg(0), reg(0));
      mov32(mem(sreg(2), vectorBase + (8 - E)), reg(0));
    } else if(size == 4) {
      mov32(reg(0), memReg2(DmemReg, reg(1)));
      rev32(reg(0), reg(0));
      mov32(mem(sreg(2), vectorBase + (12 - E)), reg(0));
    } else if(size == 2) {
      mov32_u16(reg(0), memReg2(DmemReg, reg(1)));
      rev32_u16(reg(0), reg(0));
      mov64_u16(mem(sreg(2), vectorBase + (14 - E)), reg(0));
    } else {
      mov32_u8(reg(0), memReg2(DmemReg, reg(1)));
      mov64_u8(mem(sreg(2), vectorBase + (15 - E)), reg(0));
    }
    if(slow) { deferSlowPath(slow); return; }
    return;
  }

  //LQV Vt(e),Rs,i7
  case 0x04: {
    // JIT pseudocode:
    // if(emitSlowPath) { LQV(Vt, Rs, i7); return; }
    // if(Rs is constant) {
    //   address = u12(constRs + i7 * 16);
    //   if(address > 0x0ff0) { LQV(Vt, Rs, i7); return; }  // wrap case
    //   source = dmem_data + address;
    //   target = &Vt.byte(e);
    //   if(e == 0) {
    //     if((address & 15) == 0) fastLQV0Simd(vt, source);
    //     else                    fastLQVTable(16 - (address & 15), target, source);
    //   } else {
    //     size = min(16 - (address & 15), 16 - e);
    //     fastLQVTable(size, target, source);
    //   }
    //   return;
    // }
    // address = u12(Rs + i7 * 16);
    // if(address > 0x0ff0) defer slow: LQV(Vt, Rs, i7);
    // source = dmem_data + address;
    // target = &Vt.byte(e);
    // if(e == 0) {
    //   if((address & 15) == 0) fastLQV0Simd(vt, source);
    //   else                    fastLQVTable(16 - (address & 15), target, source);
    // } else {
    //   size = min(16 - (address & 15), 16 - e);
    //   fastLQVTable(size, target, source);
    // }
    if(emitSlowPath) {
      callvu(&RSP::LQV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff0) {
        callvu(&RSP::LQV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      add64(reg(3), DmemReg, imm(address));
      add64(reg(0), sreg(2), imm(offsetof(VU, r[0]) + Vtn * sizeof(r128) + (15 - E)));
      if(E == 0) {
        if((address & 15) == 0) callf(&RSP::fastLQV0Simd, mem(Vt), reg(3));
        else                    callf(&RSP::fastLQVTable, imm(16 - (address & 15)), reg(0), reg(3));
      } else {
        u32 size = 16 - (address & 15);
        if(size > 16 - E) size = 16 - E;
        callf(&RSP::fastLQVTable, imm(size), reg(0), reg(3));
      }
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff0), set_ugt);
    auto slow = jump(flag_ugt);
    add64(reg(3), DmemReg, reg(1));
    add64(reg(0), sreg(2), imm(offsetof(VU, r[0]) + Vtn * sizeof(r128) + (15 - E)));
    if(E == 0) {
      and32(reg(2), reg(1), imm(0x0f));
      cmp32(reg(2), imm(0), set_z);
      auto simd = jump(flag_z);
      sub32(reg(2), imm(16), reg(2));
      callf(&RSP::fastLQVTable, reg(2), reg(0), reg(3));
      auto doneFast = jump();
      setLabel(simd);
      callf(&RSP::fastLQV0Simd, mem(Vt), reg(3));
      setLabel(doneFast);
    } else {
      and32(reg(2), reg(1), imm(0x0f));
      sub32(reg(2), imm(16), reg(2));
      cmp32(reg(2), imm(16 - E), set_ugt);
      auto clamp = jump(flag_ugt);
      auto sizeDone = jump();
      setLabel(clamp);
      mov32(reg(2), imm(16 - E));
      setLabel(sizeDone);
      callf(&RSP::fastLQVTable, reg(2), reg(0), reg(3));
    }
    deferSlowPath(slow);
    return;
  }

  //LRV Vt(e),Rs,i7
  case 0x05: {
    if(emitSlowPath) {
      callvu(&RSP::LRV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      u32 size = (address & 15) > E ? (address & 15) - E : 0;
      if(!size) return;
      add64(reg(3), DmemReg, imm(address & ~15));
      callf(&RSP::fastLRV, mem(Vt), reg(3), imm(size));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    and32(reg(0), reg(1), imm(0x0f));
    if(E != 0) {
      cmp32(reg(0), imm(E), set_ult);
      auto zero = jump(flag_ult);
      sub32(reg(0), reg(0), imm(E));
      auto sizeDone = jump();
      setLabel(zero);
      mov32(reg(0), imm(0));
      setLabel(sizeDone);
    }
    and32(reg(2), reg(1), imm(0xff0));
    add64(reg(3), DmemReg, reg(2));
    callf(&RSP::fastLRV, mem(Vt), reg(3), reg(0));
    return;
  }

  //LPV Vt(e),Rs,i7
  case 0x06: {
    if(emitSlowPath) {
      callvu(&RSP::LPV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 8));
      if(address > 0x0ff7) {
        callvu(&RSP::LPV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      add64(reg(3), DmemReg, imm(address & ~7));
      callf(&RSP::fastLPV, mem(Vt), reg(3), imm(u8((address & 7) - E) & 15));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 8));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    and32(reg(0), reg(1), imm(0x07));
    if(E != 0) {
      sub32(reg(0), reg(0), imm(E));
      and32(reg(0), reg(0), imm(0x0f));
    }
    and32(reg(2), reg(1), imm(0xff8));
    add64(reg(3), DmemReg, reg(2));
    callf(&RSP::fastLPV, mem(Vt), reg(3), reg(0));
    deferSlowPath(slow);
    return;
  }

  //LUV Vt(e),Rs,i7
  case 0x07: {
    if(emitSlowPath) {
      callvu(&RSP::LUV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 8));
      if(address > 0x0ff7) {
        callvu(&RSP::LUV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      add64(reg(3), DmemReg, imm(address & ~7));
      callf(&RSP::fastLUV, mem(Vt), reg(3), imm(u8((address & 7) - E) & 15));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 8));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    and32(reg(0), reg(1), imm(0x07));
    if(E != 0) {
      sub32(reg(0), reg(0), imm(E));
      and32(reg(0), reg(0), imm(0x0f));
    }
    and32(reg(2), reg(1), imm(0xff8));
    add64(reg(3), DmemReg, reg(2));
    callf(&RSP::fastLUV, mem(Vt), reg(3), reg(0));
    deferSlowPath(slow);
    return;
  }

  //LHV Vt(e),Rs,i7
  case 0x08: {
    if(emitSlowPath) {
      callvu(&RSP::LHV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff7) {
        callvu(&RSP::LHV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      add64(reg(3), DmemReg, imm(address & ~7));
      callf(&RSP::fastLHV, mem(Vt), reg(3), imm(u8((address & 7) - E) & 15));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    and32(reg(0), reg(1), imm(0x07));
    if(E != 0) {
      sub32(reg(0), reg(0), imm(E));
      and32(reg(0), reg(0), imm(0x0f));
    }
    and32(reg(2), reg(1), imm(0xff8));
    add64(reg(3), DmemReg, reg(2));
    callf(&RSP::fastLHV, mem(Vt), reg(3), reg(0));
    deferSlowPath(slow);
    return;
  }

  //LFV Vt(e),Rs,i7
  case 0x09: {
    if(emitSlowPath) {
      callvu(&RSP::LFV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff7) {
        callvu(&RSP::LFV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      u32 base = address & 7;
      u32 index = u8(base - E) & 15;
      u32 code = base | index << 4;
      add64(reg(3), DmemReg, imm(address & ~7));
      callf(&RSP::fastLFV, mem(Vt), reg(3), imm(code));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    and32(reg(0), reg(1), imm(0x07));
    mov32(reg(2), reg(0));
    if(E != 0) {
      sub32(reg(2), reg(2), imm(E));
      and32(reg(2), reg(2), imm(0x0f));
    }
    shl32(reg(2), reg(2), imm(4));
    or32(reg(2), reg(2), reg(0));
    and32(reg(0), reg(1), imm(0xff8));
    add64(reg(3), DmemReg, reg(0));
    callf(&RSP::fastLFV, mem(Vt), reg(3), reg(2));
    deferSlowPath(slow);
    return;
  }

  //LWV (not present on N64 RSP)
  case 0x0a: {
    return;
  }

  //LTV Vt(e),Rs,i7
  case 0x0b: {
    if(emitSlowPath) {
      callvu(&RSP::LTV, imm(Vtn), mem(Rs), imm(i7));
      return;
    }
    auto vectorBase = offsetof(VU, r[0]) + (Vtn & ~7) * sizeof(r128);
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff7) {
        callvu(&RSP::LTV, imm(Vtn), mem(Rs), imm(i7));
        return;
      }
      add64(reg(0), sreg(2), imm(vectorBase));
      callvu(&RSP::fastLTV, reg(0), imm(address));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    add64(reg(0), sreg(2), imm(vectorBase));
    callvu(&RSP::fastLTV, reg(0), reg(1));
    deferSlowPath(slow);
    return;
  }

  //INVALID
  case range20(0x0c, 0x1f): {
    return;
  }
  }
  #undef E
  #undef i7

  return;
}

auto RSP::Recompiler::emitSWC2(u32 instruction, u32 pc, bool delaySlot, bool emitSlowPath, u32 slowPathClocks) -> void {
  auto memReg2 = [&](const op_base& base, const op_base& index) -> op_base {
    return {SLJIT_MEM2(base.fst, index.fst), 0};
  };
  auto deferSlowPath = [&](sljit_jump* enter) -> void {
    auto& slow = slowPaths.emplace_back();
    slow.enter = enter;
    slow.resume = sljit_emit_label(compiler);
    slow.instruction = instruction;
    slow.pc = pc;
    slow.delaySlot = delaySlot;
    slow.clocks = slowPathClocks;
  };
  #define E  (instruction >> 7 & 15)
  #define i7 (s8(instruction << 1) >> 1)
  switch(instruction >> 11 & 0x1f) {

  //SBV Vt(e),Rs,i7
  case 0x00: {
    if(constRegs.has(Rsn)) mov32(reg(1), imm(u32(u12(constRegs.get(Rsn) + i7))));
    else                   add32(reg(1), mem(Rs), imm(i7)), and32(reg(1), reg(1), imm(0x0fff));
    mov32_u8(reg(0), mem(sreg(2), offsetof(VU, r[0]) + Vtn * sizeof(r128) + (15 - E)));
    mov64_u8(memReg2(DmemReg, reg(1)), reg(0));
    return;
  }

  //SSV Vt(e),Rs,i7
  case 0x01: {
    if(emitSlowPath) {
      callvu(&RSP::SSV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    auto vectorBase = offsetof(VU, r[0]) + Vtn * sizeof(r128);
    if(constRegs.has(Rsn)) {
      u32 addr = u32(u12(constRegs.get(Rsn) + i7 * 2));
      if(addr > 0x0ffe) {
        callvu(&RSP::SSV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      mov32(reg(1), imm(addr));
    } else {
      add32(reg(1), mem(Rs), imm(i7 * 2));
      and32(reg(1), reg(1), imm(0x0fff));
      cmp32(reg(1), imm(0x0ffe), set_ugt);
    }
    sljit_jump* slow = nullptr;
    if(!constRegs.has(Rsn)) slow = jump(flag_ugt);
    if(E != 15) {
      mov32_u16(reg(0), mem(sreg(2), vectorBase + (14 - E)));
      rev32_u16(reg(0), reg(0));
      mov64_u16(memReg2(DmemReg, reg(1)), reg(0));
    } else {
      mov32_u8(reg(0), mem(sreg(2), vectorBase + (15 - ((E + 0) & 15))));
      mov64_u8(memReg2(DmemReg, reg(1)), reg(0));
      add32(reg(1), reg(1), imm(1));
      mov32_u8(reg(0), mem(sreg(2), vectorBase + (15 - ((E + 1) & 15))));
      mov64_u8(memReg2(DmemReg, reg(1)), reg(0));
    }
    if(slow) { deferSlowPath(slow); return; }
    return;
  }

  //SLV Vt(e),Rs,i7
  case 0x02: {
    if(emitSlowPath) {
      callvu(&RSP::SLV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    auto vectorBase = offsetof(VU, r[0]) + Vtn * sizeof(r128);
    if(constRegs.has(Rsn)) {
      u32 addr = u32(u12(constRegs.get(Rsn) + i7 * 4));
      if(addr > 0x0ffc) {
        callvu(&RSP::SLV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      mov32(reg(1), imm(addr));
    } else {
      add32(reg(1), mem(Rs), imm(i7 * 4));
      and32(reg(1), reg(1), imm(0x0fff));
      cmp32(reg(1), imm(0x0ffc), set_ugt);
    }
    sljit_jump* slow = nullptr;
    if(!constRegs.has(Rsn)) slow = jump(flag_ugt);
    if(E <= 12) {
      mov32(reg(0), mem(sreg(2), vectorBase + (12 - E)));
      rev32(reg(0), reg(0));
      mov32(memReg2(DmemReg, reg(1)), reg(0));
    } else {
      mov32_u8(reg(0), mem(sreg(2), vectorBase + (15 - ((E + 0) & 15))));
      mov64_u8(memReg2(DmemReg, reg(1)), reg(0));
      add32(reg(1), reg(1), imm(1));
      mov32_u8(reg(0), mem(sreg(2), vectorBase + (15 - ((E + 1) & 15))));
      mov64_u8(memReg2(DmemReg, reg(1)), reg(0));
      add32(reg(1), reg(1), imm(1));
      mov32_u8(reg(0), mem(sreg(2), vectorBase + (15 - ((E + 2) & 15))));
      mov64_u8(memReg2(DmemReg, reg(1)), reg(0));
      add32(reg(1), reg(1), imm(1));
      mov32_u8(reg(0), mem(sreg(2), vectorBase + (15 - ((E + 3) & 15))));
      mov64_u8(memReg2(DmemReg, reg(1)), reg(0));
    }
    if(slow) { deferSlowPath(slow); return; }
    return;
  }

  //SDV Vt(e),Rs,i7
  case 0x03: {
    if(emitSlowPath) {
      callvu(&RSP::SDV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(E > 8) {
      callvu(&RSP::SDV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    auto vectorBase = offsetof(VU, r[0]) + Vtn * sizeof(r128);
    if(constRegs.has(Rsn)) {
      u32 addr = u32(u12(constRegs.get(Rsn) + i7 * 8));
      if(addr > 0x0ff8) {
        callvu(&RSP::SDV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      mov32(reg(1), imm(addr));
    } else {
      add32(reg(1), mem(Rs), imm(i7 * 8));
      and32(reg(1), reg(1), imm(0x0fff));
      cmp32(reg(1), imm(0x0ff8), set_ugt);
    }
    sljit_jump* slow = nullptr;
    if(!constRegs.has(Rsn)) slow = jump(flag_ugt);
    mov32(reg(0), mem(sreg(2), vectorBase + (12 - E)));
    rev32(reg(0), reg(0));
    mov32(memReg2(DmemReg, reg(1)), reg(0));
    add32(reg(1), reg(1), imm(4));
    mov32(reg(0), mem(sreg(2), vectorBase + (8 - E)));
    rev32(reg(0), reg(0));
    mov32(memReg2(DmemReg, reg(1)), reg(0));
    if(slow) { deferSlowPath(slow); return; }
    return;
  }

  //SQV Vt(e),Rs,i7
  case 0x04: {
    // JIT pseudocode:
    // if(emitSlowPath) { SQV(Vt, Rs, i7); return; }
    // if(Rs is constant) {
    //   address = u12(constRs + i7 * 16);
    //   if(address > 0x0ff0) { SQV(Vt, Rs, i7); return; }  // wrap case
    //   if(e != 0) { fastSQV<e>(Vt, address); return; }
    //   size = 16 - (address & 15);
    //   if(size == 16) fastSQV0Simd(&Vt, dmem_data + address);
    //   else           fastSQV<0>(Vt, address);
    //   return;
    // }
    // address = u12(Rs + i7 * 16);
    // if(address > 0x0ff0) defer slow: SQV(Vt, Rs, i7);
    // if(e != 0) { fastSQV<e>(Vt, address); return; }
    // size = 16 - (address & 15);
    // if(size == 16) fastSQV0Simd(&Vt, dmem_data + address);
    // else           fastSQV<0>(Vt, address);
    if(emitSlowPath) {
      callvu(&RSP::SQV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    auto vectorBase = offsetof(VU, r[0]) + Vtn * sizeof(r128);
    if(constRegs.has(Rsn)) {
      u32 constantAddress = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(constantAddress > 0x0ff0) {
        callvu(&RSP::SQV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      if(E != 0) {
        callvu(&RSP::fastSQV, mem(Vt), imm(constantAddress));
        return;
      }
      u32 size = 16 - (constantAddress & 15);
      if(size == 16) {
        add64(reg(3), DmemReg, imm(constantAddress));
        add64(reg(0), sreg(2), imm(vectorBase));
        callf(&RSP::fastSQV0Simd, reg(0), reg(3));
      } else {
        callvu(&RSP::fastSQV, mem(Vt), imm(constantAddress));
      }
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff0), set_ugt);
    auto slow = jump(flag_ugt);
    if(E != 0) {
      callvu(&RSP::fastSQV, mem(Vt), reg(1));
      deferSlowPath(slow);
      return;
    }
    add64(reg(3), DmemReg, reg(1));
    and32(reg(0), reg(1), imm(0x0f));
    sub32(reg(0), imm(16), reg(0));
    cmp32(reg(0), imm(16), set_z);
    auto simd = jump(flag_z);
    callvu(&RSP::fastSQV, mem(Vt), reg(1));
    auto doneFast = jump();
    setLabel(simd);
    add64(reg(0), sreg(2), imm(vectorBase));
    callf(&RSP::fastSQV0Simd, reg(0), reg(3));
    setLabel(doneFast);
    deferSlowPath(slow);
    return;
  }

  //SRV Vt(e),Rs,i7
  case 0x05: {
    if(emitSlowPath) {
      callvu(&RSP::SRV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      u32 size = address & 15;
      if(!size) return;
      u32 start = u8(E - size) & 15;
      u32 code = size | start << 4;
      add64(reg(3), DmemReg, imm(address & ~15));
      callf(&RSP::fastSRV, mem(Vt), reg(3), imm(code));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    and32(reg(0), reg(1), imm(0x0f));
    mov32(reg(2), imm(E));
    sub32(reg(2), reg(2), reg(0));
    and32(reg(2), reg(2), imm(0x0f));
    shl32(reg(2), reg(2), imm(4));
    or32(reg(2), reg(2), reg(0));
    and32(reg(1), reg(1), imm(0xff0));
    add64(reg(3), DmemReg, reg(1));
    callf(&RSP::fastSRV, mem(Vt), reg(3), reg(2));
    return;
  }

  //SPV Vt(e),Rs,i7
  case 0x06: {
    if(emitSlowPath) {
      callvu(&RSP::SPV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    auto source = (u8 const*)simdSUVSource[(E + 8) & 15];
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 8));
      if(address > 0x0ff8) {
        callvu(&RSP::SPV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      add64(reg(3), DmemReg, imm(address));
      callf(&RSP::fastSUV, mem(Vt), reg(3), imm64(source));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 8));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff8), set_ugt);
    auto slow = jump(flag_ugt);
    add64(reg(3), DmemReg, reg(1));
    callf(&RSP::fastSUV, mem(Vt), reg(3), imm64(source));
    deferSlowPath(slow);
    return;
  }

  //SUV Vt(e),Rs,i7
  case 0x07: {
    if(emitSlowPath) {
      callvu(&RSP::SUV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    auto source = (u8 const*)simdSUVSource[E & 15];
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 8));
      if(address > 0x0ff8) {
        callvu(&RSP::SUV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      add64(reg(3), DmemReg, imm(address));
      callf(&RSP::fastSUV, mem(Vt), reg(3), imm64(source));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 8));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff8), set_ugt);
    auto slow = jump(flag_ugt);
    add64(reg(3), DmemReg, reg(1));
    callf(&RSP::fastSUV, mem(Vt), reg(3), imm64(source));
    deferSlowPath(slow);
    return;
  }

  //SHV Vt(e),Rs,i7
  case 0x08: {
    if(emitSlowPath) {
      callvu(&RSP::SHV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff7) {
        callvu(&RSP::SHV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      u32 code = (address & 7) | E << 4;
      add64(reg(3), DmemReg, imm(address & ~7));
      callf(&RSP::fastSHV, mem(Vt), reg(3), imm(code));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    and32(reg(0), reg(1), imm(0x07));
    if(E != 0) {
      or32(reg(0), reg(0), imm(E << 4));
    }
    and32(reg(2), reg(1), imm(0xff8));
    add64(reg(3), DmemReg, reg(2));
    callf(&RSP::fastSHV, mem(Vt), reg(3), reg(0));
    deferSlowPath(slow);
    return;
  }

  //SFV Vt(e),Rs,i7
  case 0x09: {
    if(emitSlowPath) {
      callvu(&RSP::SFV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff7) {
        callvu(&RSP::SFV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      u32 code = (address & 7) | E << 4;
      add64(reg(3), DmemReg, imm(address & ~7));
      callf(&RSP::fastSFV, mem(Vt), reg(3), imm(code));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    and32(reg(0), reg(1), imm(0x07));
    if(E != 0) {
      or32(reg(0), reg(0), imm(E << 4));
    }
    and32(reg(2), reg(1), imm(0xff8));
    add64(reg(3), DmemReg, reg(2));
    callf(&RSP::fastSFV, mem(Vt), reg(3), reg(0));
    deferSlowPath(slow);
    return;
  }

  //SWV Vt(e),Rs,i7
  case 0x0a: {
    if(emitSlowPath) {
      callvu(&RSP::SWV, mem(Vt), mem(Rs), imm(i7));
      return;
    }
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff7) {
        callvu(&RSP::SWV, mem(Vt), mem(Rs), imm(i7));
        return;
      }
      u32 base = address & 7;
      u32 index = u8(base - E) & 15;
      u32 code = base | index << 4;
      add64(reg(3), DmemReg, imm(address & ~7));
      callf(&RSP::fastSWV, mem(Vt), reg(3), imm(code));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    and32(reg(0), reg(1), imm(0x07));
    mov32(reg(2), reg(0));
    if(E != 0) {
      sub32(reg(2), reg(2), imm(E));
      and32(reg(2), reg(2), imm(0x0f));
    }
    shl32(reg(2), reg(2), imm(4));
    or32(reg(2), reg(2), reg(0));
    and32(reg(0), reg(1), imm(0xff8));
    add64(reg(3), DmemReg, reg(0));
    callf(&RSP::fastSWV, mem(Vt), reg(3), reg(2));
    deferSlowPath(slow);
    return;
  }

  //STV Vt(e),Rs,i7
  case 0x0b: {
    if(emitSlowPath) {
      callvu(&RSP::STV, imm(Vtn), mem(Rs), imm(i7));
      return;
    }
    auto vectorBase = offsetof(VU, r[0]) + (Vtn & ~7) * sizeof(r128);
    if(constRegs.has(Rsn)) {
      u32 address = u32(u12(constRegs.get(Rsn) + i7 * 16));
      if(address > 0x0ff7) {
        callvu(&RSP::STV, imm(Vtn), mem(Rs), imm(i7));
        return;
      }
      add64(reg(0), sreg(2), imm(vectorBase));
      callvu(&RSP::fastSTV, reg(0), imm(address));
      return;
    }
    add32(reg(1), mem(Rs), imm(i7 * 16));
    and32(reg(1), reg(1), imm(0x0fff));
    cmp32(reg(1), imm(0x0ff7), set_ugt);
    auto slow = jump(flag_ugt);
    add64(reg(0), sreg(2), imm(vectorBase));
    callvu(&RSP::fastSTV, reg(0), reg(1));
    deferSlowPath(slow);
    return;
  }
  //INVALID
  case range20(0x0c, 0x1f): {
    return;
  }

  }
  #undef E
  #undef i7

  return;
}

#if ARCHITECTURE_SUPPORTS_SSE4_1
alignas(16) static constexpr u8 simdShuffleIdentity[16] = {
  15, 14, 13, 12, 11, 10,  9,  8,
   7,  6,  5,  4,  3,  2,  1,  0,
};
alignas(16) static constexpr u8 simdShuffleSwapWords[16] = {
  14, 15, 12, 13, 10, 11,  8,  9,
   6,  7,  4,  5,  2,  3,  0,  1,
};
alignas(16) static constexpr u8 simdShuffleSwapBytes[16] = {
   1,  0,  3,  2,  5,  4,  7,  6,
   9,  8, 11, 10, 13, 12, 15, 14,
};
#endif

alignas(16) u8 const simdLHVSource[16][8] = {
  { 0,  1,  2,  3,  4,  5,  6,  7},
  {15,  0,  1,  2,  3,  4,  5,  6},
  {14, 15,  0,  1,  2,  3,  4,  5},
  {13, 14, 15,  0,  1,  2,  3,  4},
  {12, 13, 14, 15,  0,  1,  2,  3},
  {11, 12, 13, 14, 15,  0,  1,  2},
  {10, 11, 12, 13, 14, 15,  0,  1},
  { 9, 10, 11, 12, 13, 14, 15,  0},
  { 8,  9, 10, 11, 12, 13, 14, 15},
  { 7,  8,  9, 10, 11, 12, 13, 14},
  { 6,  7,  8,  9, 10, 11, 12, 13},
  { 5,  6,  7,  8,  9, 10, 11, 12},
  { 4,  5,  6,  7,  8,  9, 10, 11},
  { 3,  4,  5,  6,  7,  8,  9, 10},
  { 2,  3,  4,  5,  6,  7,  8,  9},
  { 1,  2,  3,  4,  5,  6,  7,  8},
};

alignas(16) u8 const simdLFVData[16][32] = {
  { 0,  1,  2,  3,  4,  5,  6,  7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0,    0,    0,    0,    0,    0,    0},
  {15,  0,  1,  2,  3,  4,  5,  6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0,    0,    0,    0,    0,    0},
  {14, 15,  0,  1,  2,  3,  4,  5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0,    0,    0,    0,    0},
  {13, 14, 15,  0,  1,  2,  3,  4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0,    0,    0,    0},
  {12, 13, 14, 15,  0,  1,  2,  3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0,    0,    0},
  {11, 12, 13, 14, 15,  0,  1,  2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0,    0},
  {10, 11, 12, 13, 14, 15,  0,  1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0},
  { 9, 10, 11, 12, 13, 14, 15,  0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0},
  { 8,  9, 10, 11, 12, 13, 14, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
  { 7,  8,  9, 10, 11, 12, 13, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
  { 6,  7,  8,  9, 10, 11, 12, 13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
  { 5,  6,  7,  8,  9, 10, 11, 12, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff, 0xff},
  { 4,  5,  6,  7,  8,  9, 10, 11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff, 0xff},
  { 3,  4,  5,  6,  7,  8,  9, 10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff, 0xff, 0xff},
  { 2,  3,  4,  5,  6,  7,  8,  9, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff, 0xff},
  { 1,  2,  3,  4,  5,  6,  7,  8, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff},
};

alignas(16) u8 const simdLTVData[16][32] = {
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7},
  { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,
    9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8},
  { 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,
   10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
  { 3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,
   11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10},
  { 4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,
   12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11},
  { 5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,
   13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
  { 6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,
   14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13},
  { 7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,
   15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14},
  { 8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
  { 9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,
    1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0},
  {10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1},
  {11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
    3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2},
  {12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
    4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3},
  {13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,
    5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4},
  {14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
    6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5},
  {15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6},
};

alignas(16) u8 const simdLRVSize[16][16] = {
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
  { 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14},
  { 0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13},
  { 0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
  { 0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11},
  { 0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10},
  { 0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
  { 0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1},
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
};

alignas(16) u8 const simdSFVSource[16][4] = {
  {0x00, 0x01, 0x02, 0x03},
  {0x06, 0x07, 0x04, 0x05},
  {0xff, 0xff, 0xff, 0xff},
  {0xff, 0xff, 0xff, 0xff},
  {0x01, 0x02, 0x03, 0x00},
  {0x07, 0x04, 0x05, 0x06},
  {0xff, 0xff, 0xff, 0xff},
  {0xff, 0xff, 0xff, 0xff},
  {0x04, 0x05, 0x06, 0x07},
  {0xff, 0xff, 0xff, 0xff},
  {0xff, 0xff, 0xff, 0xff},
  {0x03, 0x00, 0x01, 0x02},
  {0x05, 0x06, 0x07, 0x04},
  {0xff, 0xff, 0xff, 0xff},
  {0xff, 0xff, 0xff, 0xff},
  {0x00, 0x01, 0x02, 0x03},
};

alignas(16) u8 const simdSRVStart[16][16] = {
  { 0, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1},
  { 1,  0, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2},
  { 2,  1,  0, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3},
  { 3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4},
  { 4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5},
  { 5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6},
  { 6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9,  8,  7},
  { 7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9,  8},
  { 8,  7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10,  9},
  { 9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11, 10},
  {10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12, 11},
  {11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13, 12},
  {12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 15, 14, 13},
  {13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 15, 14},
  {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 15},
  {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0},
};

alignas(16) u8 const simdSUVSource[16][8] = {
  { 0,  1,  2,  3,  4,  5,  6,  7},
  { 1,  2,  3,  4,  5,  6,  7,  8},
  { 2,  3,  4,  5,  6,  7,  8,  9},
  { 3,  4,  5,  6,  7,  8,  9, 10},
  { 4,  5,  6,  7,  8,  9, 10, 11},
  { 5,  6,  7,  8,  9, 10, 11, 12},
  { 6,  7,  8,  9, 10, 11, 12, 13},
  { 7,  8,  9, 10, 11, 12, 13, 14},
  { 8,  9, 10, 11, 12, 13, 14, 15},
  { 9, 10, 11, 12, 13, 14, 15,  0},
  {10, 11, 12, 13, 14, 15,  0,  1},
  {11, 12, 13, 14, 15,  0,  1,  2},
  {12, 13, 14, 15,  0,  1,  2,  3},
  {13, 14, 15,  0,  1,  2,  3,  4},
  {14, 15,  0,  1,  2,  3,  4,  5},
  {15,  0,  1,  2,  3,  4,  5,  6},
};

alignas(16) u8 const simdSWVSource[16][8] = {
  { 0,  1,  2,  3,  4,  5,  6,  7},
  {15,  0,  1,  2,  3,  4,  5,  6},
  {14, 15,  0,  1,  2,  3,  4,  5},
  {13, 14, 15,  0,  1,  2,  3,  4},
  {12, 13, 14, 15,  0,  1,  2,  3},
  {11, 12, 13, 14, 15,  0,  1,  2},
  {10, 11, 12, 13, 14, 15,  0,  1},
  { 9, 10, 11, 12, 13, 14, 15,  0},
  { 8,  9, 10, 11, 12, 13, 14, 15},
  { 7,  8,  9, 10, 11, 12, 13, 14},
  { 6,  7,  8,  9, 10, 11, 12, 13},
  { 5,  6,  7,  8,  9, 10, 11, 12},
  { 4,  5,  6,  7,  8,  9, 10, 11},
  { 3,  4,  5,  6,  7,  8,  9, 10},
  { 2,  3,  4,  5,  6,  7,  8,  9},
  { 1,  2,  3,  4,  5,  6,  7,  8},
};

alignas(16) u8 const simdSHVSource[16][32] = {
  { 0,  2,  4,  6,  8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    1,  3,  5,  7,  9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 1,  3,  5,  7,  9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    2,  4,  6,  8, 10, 12, 14,  0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 2,  4,  6,  8, 10, 12, 14,  0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    3,  5,  7,  9, 11, 13, 15,  1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 3,  5,  7,  9, 11, 13, 15,  1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    4,  6,  8, 10, 12, 14,  0,  2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 4,  6,  8, 10, 12, 14,  0,  2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    5,  7,  9, 11, 13, 15,  1,  3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 5,  7,  9, 11, 13, 15,  1,  3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    6,  8, 10, 12, 14,  0,  2,  4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 6,  8, 10, 12, 14,  0,  2,  4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    7,  9, 11, 13, 15,  1,  3,  5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 7,  9, 11, 13, 15,  1,  3,  5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    8, 10, 12, 14,  0,  2,  4,  6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 8, 10, 12, 14,  0,  2,  4,  6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    9, 11, 13, 15,  1,  3,  5,  7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 9, 11, 13, 15,  1,  3,  5,  7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   10, 12, 14,  0,  2,  4,  6,  8, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {10, 12, 14,  0,  2,  4,  6,  8, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   11, 13, 15,  1,  3,  5,  7,  9, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {11, 13, 15,  1,  3,  5,  7,  9, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   12, 14,  0,  2,  4,  6,  8, 10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {12, 14,  0,  2,  4,  6,  8, 10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   13, 15,  1,  3,  5,  7,  9, 11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {13, 15,  1,  3,  5,  7,  9, 11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   14,  0,  2,  4,  6,  8, 10, 12, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {14,  0,  2,  4,  6,  8, 10, 12, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   15,  1,  3,  5,  7,  9, 11, 13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {15,  1,  3,  5,  7,  9, 11, 13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0,  2,  4,  6,  8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
};

#if ARCHITECTURE_SUPPORTS_SSE4_1
alignas(16) static constexpr u8 simdSHVDestination[8][16] = {
  {0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80},
  {0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7},
  {7, 0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80},
  {0x80, 7, 0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6},
  {6, 0x80, 7, 0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80},
  {0x80, 6, 0x80, 7, 0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5},
  {5, 0x80, 6, 0x80, 7, 0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80},
  {0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4},
};
alignas(16) static constexpr u8 simdSHVMask[8][16] = {
  {0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0},
  {0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff},
  {0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0},
  {0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff},
  {0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0},
  {0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff},
  {0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0},
  {0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0, 0xff},
};

alignas(16) static constexpr u8 simdSTVDestination[16][16] = {
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
  {15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14},
  {14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13},
  {13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
  {12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11},
  {11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10},
  {10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
  { 9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7,  8},
  { 8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6,  7},
  { 7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5,  6},
  { 6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4,  5},
  { 5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3,  4},
  { 4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3},
  { 3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2},
  { 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1},
  { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0},
};

alignas(16) static constexpr u8 simdLFVSource[16][16] = {
  { 0,  4,  8, 12,  8, 12,  0,  4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 1,  5,  9, 13,  9, 13,  1,  5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 2,  6, 10, 14, 10, 14,  2,  6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 3,  7, 11, 15, 11, 15,  3,  7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 4,  8, 12,  0, 12,  0,  4,  8, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 5,  9, 13,  1, 13,  1,  5,  9, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 6, 10, 14,  2, 14,  2,  6, 10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 7, 11, 15,  3, 15,  3,  7, 11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 8, 12,  0,  4,  0,  4,  8, 12, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  { 9, 13,  1,  5,  1,  5,  9, 13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {10, 14,  2,  6,  2,  6, 10, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {11, 15,  3,  7,  3,  7, 11, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {12,  0,  4,  8,  4,  8, 12,  0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {13,  1,  5,  9,  5,  9, 13,  1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {14,  2,  6, 10,  6, 10, 14,  2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
  {15,  3,  7, 11,  7, 11, 15,  3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
};

alignas(16) static constexpr u8 simdSFVDestination[8][16] = {
  {0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80, 0x80, 0x80, 3, 0x80, 0x80, 0x80},
  {0x80, 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80, 0x80, 0x80, 3, 0x80, 0x80},
  {0x80, 0x80, 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80, 0x80, 0x80, 3, 0x80},
  {0x80, 0x80, 0x80, 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80, 0x80, 0x80, 3},
  {3, 0x80, 0x80, 0x80, 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80, 0x80, 0x80},
  {0x80, 3, 0x80, 0x80, 0x80, 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80, 0x80},
  {0x80, 0x80, 3, 0x80, 0x80, 0x80, 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2, 0x80},
  {0x80, 0x80, 0x80, 3, 0x80, 0x80, 0x80, 0, 0x80, 0x80, 0x80, 1, 0x80, 0x80, 0x80, 2},
};

alignas(16) static constexpr u8 simdSFVMask[8][16] = {
  {0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0},
  {0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0},
  {0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0},
  {0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff},
  {0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0},
  {0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0},
  {0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0},
  {0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0, 0, 0, 0xff},
};
#endif

auto RSP::fastLQVTable(u32 size, u8* target, u8* source) -> void {
  switch(size) {
  case 16: target[-15] = source[15]; [[fallthrough]];
  case 15: target[-14] = source[14]; [[fallthrough]];
  case 14: target[-13] = source[13]; [[fallthrough]];
  case 13: target[-12] = source[12]; [[fallthrough]];
  case 12: target[-11] = source[11]; [[fallthrough]];
  case 11: target[-10] = source[10]; [[fallthrough]];
  case 10: target[ -9] = source[ 9]; [[fallthrough]];
  case  9: target[ -8] = source[ 8]; [[fallthrough]];
  case  8: target[ -7] = source[ 7]; [[fallthrough]];
  case  7: target[ -6] = source[ 6]; [[fallthrough]];
  case  6: target[ -5] = source[ 5]; [[fallthrough]];
  case  5: target[ -4] = source[ 4]; [[fallthrough]];
  case  4: target[ -3] = source[ 3]; [[fallthrough]];
  case  3: target[ -2] = source[ 2]; [[fallthrough]];
  case  2: target[ -1] = source[ 1]; [[fallthrough]];
  case  1: target[  0] = source[ 0]; [[fallthrough]];
  default: return;
  }
}

auto RSP::fastLQV0Simd(r128& vt, u8* source) -> void {
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto value = _mm_loadu_si128((const __m128i*)source);
  auto reverse = _mm_load_si128((const __m128i*)simdShuffleIdentity);
  value = _mm_shuffle_epi8(value, reverse);
  _mm_storeu_si128((__m128i*)&vt.u128, value);
#else
  vt.byte(15) = source[15];
  vt.byte(14) = source[14];
  vt.byte(13) = source[13];
  vt.byte(12) = source[12];
  vt.byte(11) = source[11];
  vt.byte(10) = source[10];
  vt.byte( 9) = source[ 9];
  vt.byte( 8) = source[ 8];
  vt.byte( 7) = source[ 7];
  vt.byte( 6) = source[ 6];
  vt.byte( 5) = source[ 5];
  vt.byte( 4) = source[ 4];
  vt.byte( 3) = source[ 3];
  vt.byte( 2) = source[ 2];
  vt.byte( 1) = source[ 1];
  vt.byte( 0) = source[ 0];
#endif
}

auto RSP::fastLRV(r128& vt, u8* source, u32 size) -> void {
  if(!size) return;
  auto target = (u8*)&vt.u128 + (size - 1);
  fastLQVTable(size, target, source);
}

auto RSP::fastLPV(r128& vt, u8 const* target, u32 index) -> void {
  if(!index) {
    fastLPV0Simd(vt, target);
    return;
  }
  for(u32 offset = 0; offset < 8; offset++) {
    vt.element(offset) = target[(index + offset) & 15] << 8;
  }
}

auto RSP::fastLPV0Simd(r128& vt, u8 const* source) -> void {
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto bytes = _mm_loadl_epi64((__m128i const*)source);
  auto words = _mm_cvtepu8_epi16(bytes);
  words = _mm_slli_epi16(words, 8);
  auto reverseWords = _mm_load_si128((const __m128i*)simdShuffleSwapWords);
  vt = _mm_shuffle_epi8(words, reverseWords);
#else
  vt.element(0) = source[0] << 8;
  vt.element(1) = source[1] << 8;
  vt.element(2) = source[2] << 8;
  vt.element(3) = source[3] << 8;
  vt.element(4) = source[4] << 8;
  vt.element(5) = source[5] << 8;
  vt.element(6) = source[6] << 8;
  vt.element(7) = source[7] << 8;
#endif
}

auto RSP::fastLUV(r128& vt, u8 const* target, u32 index) -> void {
  if(!index) {
    fastLUV0Simd(vt, target);
    return;
  }
  for(u32 offset = 0; offset < 8; offset++) {
    vt.element(offset) = target[(index + offset) & 15] << 7;
  }
}

auto RSP::fastLUV0Simd(r128& vt, u8 const* source) -> void {
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto bytes = _mm_loadl_epi64((__m128i const*)source);
  auto words = _mm_cvtepu8_epi16(bytes);
  words = _mm_slli_epi16(words, 7);
  auto reverseWords = _mm_load_si128((const __m128i*)simdShuffleSwapWords);
  vt = _mm_shuffle_epi8(words, reverseWords);
#else
  vt.element(0) = source[0] << 7;
  vt.element(1) = source[1] << 7;
  vt.element(2) = source[2] << 7;
  vt.element(3) = source[3] << 7;
  vt.element(4) = source[4] << 7;
  vt.element(5) = source[5] << 7;
  vt.element(6) = source[6] << 7;
  vt.element(7) = source[7] << 7;
#endif
}

template<u8 e>
auto RSP::fastLTV(r128* vtbase, u32 address) -> void {
  constexpr u32 vtoff = e >> 1;
  auto target = dmem.data + (address & ~7);
  auto select = simdLTVData[e] + ((address & 8) ? 16 : 0);
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto bytes = _mm_loadu_si128((const __m128i*)target);
  auto shuffle = _mm_load_si128((const __m128i*)select);
  bytes = _mm_shuffle_epi8(bytes, shuffle);
  alignas(16) u16 data[8];
  _mm_store_si128((__m128i*)data, bytes);
  vtbase[(vtoff + 0) & 7].u16(0) = bswap16(data[0]);
  vtbase[(vtoff + 1) & 7].u16(1) = bswap16(data[1]);
  vtbase[(vtoff + 2) & 7].u16(2) = bswap16(data[2]);
  vtbase[(vtoff + 3) & 7].u16(3) = bswap16(data[3]);
  vtbase[(vtoff + 4) & 7].u16(4) = bswap16(data[4]);
  vtbase[(vtoff + 5) & 7].u16(5) = bswap16(data[5]);
  vtbase[(vtoff + 6) & 7].u16(6) = bswap16(data[6]);
  vtbase[(vtoff + 7) & 7].u16(7) = bswap16(data[7]);
#else
  vtbase[(vtoff + 0) & 7].u16(0) = u16(target[select[ 0]]) << 8 | u16(target[select[ 1]]);
  vtbase[(vtoff + 1) & 7].u16(1) = u16(target[select[ 2]]) << 8 | u16(target[select[ 3]]);
  vtbase[(vtoff + 2) & 7].u16(2) = u16(target[select[ 4]]) << 8 | u16(target[select[ 5]]);
  vtbase[(vtoff + 3) & 7].u16(3) = u16(target[select[ 6]]) << 8 | u16(target[select[ 7]]);
  vtbase[(vtoff + 4) & 7].u16(4) = u16(target[select[ 8]]) << 8 | u16(target[select[ 9]]);
  vtbase[(vtoff + 5) & 7].u16(5) = u16(target[select[10]]) << 8 | u16(target[select[11]]);
  vtbase[(vtoff + 6) & 7].u16(6) = u16(target[select[12]]) << 8 | u16(target[select[13]]);
  vtbase[(vtoff + 7) & 7].u16(7) = u16(target[select[14]]) << 8 | u16(target[select[15]]);
#endif
}

auto RSP::fastLFV(r128& vt, u8 const* target, u32 code) -> void {
  u32 base = code & 15;
  u32 index = code >> 4;
  u32 e = (base - index) & 15;
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto reverse = _mm_load_si128((const __m128i*)simdShuffleIdentity);
  auto current = _mm_shuffle_epi8(vt, reverse);
  auto select = _mm_load_si128((const __m128i*)simdLFVSource[index]);
  auto bytes = _mm_loadu_si128((const __m128i*)target);
  bytes = _mm_shuffle_epi8(bytes, select);
  auto words = _mm_cvtepu8_epi16(bytes);
  words = _mm_slli_epi16(words, 7);
  auto swapBytes = _mm_load_si128((const __m128i*)simdShuffleSwapBytes);
  auto value = _mm_shuffle_epi8(words, swapBytes);
  auto mask = _mm_load_si128((const __m128i*)(simdLFVData[e] + 16));
  auto merged = _mm_andnot_si128(mask, current);
  merged = _mm_or_si128(merged, _mm_and_si128(mask, value));
  vt = _mm_shuffle_epi8(merged, reverse);
#else
  u32 start = e;
  u32 end = start + 8;
  if(end > 16) end = 16;
  r128 tmp;
  for(u32 offset = 0; offset < 4; offset++) {
    tmp.element(offset + 0) = target[(index + offset * 4 + 0) & 15] << 7;
    tmp.element(offset + 4) = target[(index + offset * 4 + 8) & 15] << 7;
  }
  for(u32 offset = start; offset < end; offset++) {
    vt.byte(offset) = tmp.byte(offset);
  }
#endif
}

auto RSP::fastLHV(r128& vt, u8 const* target, u32 index) -> void {
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto select = _mm_load_si128((const __m128i*)simdSHVSource[index]);
  auto bytes = _mm_loadu_si128((const __m128i*)target);
  bytes = _mm_shuffle_epi8(bytes, select);
  auto words = _mm_cvtepu8_epi16(bytes);
  words = _mm_slli_epi16(words, 7);
  auto reverseWords = _mm_load_si128((const __m128i*)simdShuffleSwapWords);
  vt = _mm_shuffle_epi8(words, reverseWords);
#else
  for(u32 offset = 0; offset < 8; offset++) {
    vt.element(offset) = target[(index + offset * 2) & 15] << 7;
  }
#endif
}

auto RSP::fastSQVTable(u32 size, u8* target, u8 const* source) -> void {
  switch(size) {
  case 16: target[15] = source[-15]; [[fallthrough]];
  case 15: target[14] = source[-14]; [[fallthrough]];
  case 14: target[13] = source[-13]; [[fallthrough]];
  case 13: target[12] = source[-12]; [[fallthrough]];
  case 12: target[11] = source[-11]; [[fallthrough]];
  case 11: target[10] = source[-10]; [[fallthrough]];
  case 10: target[ 9] = source[ -9]; [[fallthrough]];
  case  9: target[ 8] = source[ -8]; [[fallthrough]];
  case  8: target[ 7] = source[ -7]; [[fallthrough]];
  case  7: target[ 6] = source[ -6]; [[fallthrough]];
  case  6: target[ 5] = source[ -5]; [[fallthrough]];
  case  5: target[ 4] = source[ -4]; [[fallthrough]];
  case  4: target[ 3] = source[ -3]; [[fallthrough]];
  case  3: target[ 2] = source[ -2]; [[fallthrough]];
  case  2: target[ 1] = source[ -1]; [[fallthrough]];
  case  1: target[ 0] = source[  0]; [[fallthrough]];
  default: break;
  }
}

template<u8 e>
auto RSP::fastSQV(cr128& vt, u32 address) -> void {
  auto size = 16 - (address & 15);
  auto target = dmem.data + address;
  auto source = (u8 const*)&vt.u128 + (15 - e);
  auto first = min(size, u32(16 - e));
  fastSQVTable(first, target, source);
  if(first == size) return;
  target += first;
  source = (u8 const*)&vt.u128 + 15;
  fastSQVTable(size - first, target, source);
}

auto RSP::fastSQV0Simd(u8 const* source, u8* target) -> void {
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto value = _mm_loadu_si128((const __m128i*)source);
  auto reverse = _mm_load_si128((const __m128i*)simdShuffleIdentity);
  value = _mm_shuffle_epi8(value, reverse);
  _mm_storeu_si128((__m128i*)target, value);
#else
  target[15] = source[ 0];
  target[14] = source[ 1];
  target[13] = source[ 2];
  target[12] = source[ 3];
  target[11] = source[ 4];
  target[10] = source[ 5];
  target[ 9] = source[ 6];
  target[ 8] = source[ 7];
  target[ 7] = source[ 8];
  target[ 6] = source[ 9];
  target[ 5] = source[10];
  target[ 4] = source[11];
  target[ 3] = source[12];
  target[ 2] = source[13];
  target[ 1] = source[14];
  target[ 0] = source[15];
#endif
}

auto RSP::fastSRV(cr128& vt, u8* target, u32 code) -> void {
  u32 size = code & 15;
  u32 start = code >> 4;
  switch(size) {
  case 15: target[14] = vt.byte((start + 14) & 15); [[fallthrough]];
  case 14: target[13] = vt.byte((start + 13) & 15); [[fallthrough]];
  case 13: target[12] = vt.byte((start + 12) & 15); [[fallthrough]];
  case 12: target[11] = vt.byte((start + 11) & 15); [[fallthrough]];
  case 11: target[10] = vt.byte((start + 10) & 15); [[fallthrough]];
  case 10: target[ 9] = vt.byte((start +  9) & 15); [[fallthrough]];
  case  9: target[ 8] = vt.byte((start +  8) & 15); [[fallthrough]];
  case  8: target[ 7] = vt.byte((start +  7) & 15); [[fallthrough]];
  case  7: target[ 6] = vt.byte((start +  6) & 15); [[fallthrough]];
  case  6: target[ 5] = vt.byte((start +  5) & 15); [[fallthrough]];
  case  5: target[ 4] = vt.byte((start +  4) & 15); [[fallthrough]];
  case  4: target[ 3] = vt.byte((start +  3) & 15); [[fallthrough]];
  case  3: target[ 2] = vt.byte((start +  2) & 15); [[fallthrough]];
  case  2: target[ 1] = vt.byte((start +  1) & 15); [[fallthrough]];
  case  1: target[ 0] = vt.byte((start +  0) & 15); [[fallthrough]];
  default: break;
  }
}

auto RSP::fastSPV0Simd(cr128& vt, u8* target) -> void {
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto reverseWords = _mm_load_si128((const __m128i*)simdShuffleSwapWords);
  auto words = _mm_shuffle_epi8(vt, reverseWords);
  words = _mm_srli_epi16(words, 8);
  words = _mm_and_si128(words, _mm_set1_epi16(0x00ff));
  auto bytes = _mm_packus_epi16(words, _mm_setzero_si128());
  _mm_storel_epi64((__m128i*)target, bytes);
#else
  target[0] = vt.byte( 0);
  target[1] = vt.byte( 2);
  target[2] = vt.byte( 4);
  target[3] = vt.byte( 6);
  target[4] = vt.byte( 8);
  target[5] = vt.byte(10);
  target[6] = vt.byte(12);
  target[7] = vt.byte(14);
#endif
}

template<u8 e>
auto RSP::fastSTV(r128* vtbase, u32 address) -> void {
  constexpr u32 ee = e & ~1;
  constexpr u32 element = (16 - ee) & 15;
  constexpr u32 element16 = element >> 1;
  alignas(16) u16 data[8];
  auto& v0 = vtbase[0];
  auto& v1 = vtbase[1];
  auto& v2 = vtbase[2];
  auto& v3 = vtbase[3];
  auto& v4 = vtbase[4];
  auto& v5 = vtbase[5];
  auto& v6 = vtbase[6];
  auto& v7 = vtbase[7];
  data[0] = bswap16(v0.u16((element16 + 0) & 7));
  data[1] = bswap16(v1.u16((element16 + 1) & 7));
  data[2] = bswap16(v2.u16((element16 + 2) & 7));
  data[3] = bswap16(v3.u16((element16 + 3) & 7));
  data[4] = bswap16(v4.u16((element16 + 4) & 7));
  data[5] = bswap16(v5.u16((element16 + 5) & 7));
  data[6] = bswap16(v6.u16((element16 + 6) & 7));
  data[7] = bswap16(v7.u16((element16 + 7) & 7));
  auto target = dmem.data + (address & ~7);
  u32 base = (address & 7) - ee & 15;
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto bytes = _mm_load_si128((const __m128i*)data);
  auto destination = _mm_load_si128((const __m128i*)simdSTVDestination[base]);
  bytes = _mm_shuffle_epi8(bytes, destination);
  _mm_storeu_si128((__m128i*)target, bytes);
#else
  auto output = (u8 const*)data;
  for(u32 i = 0; i < 16; i++) {
    target[(base + i) & 15] = output[i];
  }
#endif
}

auto RSP::fastSWV(cr128& vt, u8* target, u32 code) -> void {
  u32 base = code & 15;
  u32 index = code >> 4;
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto reverse = _mm_load_si128((const __m128i*)simdShuffleIdentity);
  auto bytes = _mm_shuffle_epi8(vt, reverse);
  auto select = _mm_load_si128((const __m128i*)simdSTVDestination[index]);
  bytes = _mm_shuffle_epi8(bytes, select);
  _mm_storeu_si128((__m128i*)target, bytes);
#else
  u32 e = (base - index) & 15;
  for(u32 i = 0; i < 16; i++) {
    target[(base + i) & 15] = vt.byte((e + i) & 15);
  }
#endif
}

auto RSP::fastSUV(cr128& vt, u8* target, u8 const* source) -> void {
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto reverseWords = _mm_load_si128((const __m128i*)simdShuffleSwapWords);
  auto words = _mm_shuffle_epi8(vt, reverseWords);
  auto elements = _mm_srli_epi16(words, 7);
  elements = _mm_and_si128(elements, _mm_set1_epi16(0x00ff));
  auto elements8 = _mm_packus_epi16(elements, _mm_setzero_si128());
  auto bytes = _mm_srli_epi16(words, 8);
  bytes = _mm_and_si128(bytes, _mm_set1_epi16(0x00ff));
  auto bytes8 = _mm_packus_epi16(bytes, _mm_setzero_si128());
  auto combined = _mm_unpacklo_epi64(elements8, bytes8);
  auto select = _mm_loadl_epi64((const __m128i*)source);
  auto result = _mm_shuffle_epi8(combined, select);
  _mm_storel_epi64((__m128i*)target, result);
#else
  for(u32 offset = 0; offset < 8; offset++) {
    u32 code = source[offset];
    if(code < 8) *target++ = vt.element(code) >> 7;
    else         *target++ = vt.byte((code - 8) << 1);
  }
#endif
}

auto RSP::fastSFV(cr128& vt, u8* target, u32 code) -> void {
  u32 base = code & 15;
  auto source = simdSFVSource[code >> 4];
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto reverseWords = _mm_load_si128((const __m128i*)simdShuffleSwapWords);
  auto words = _mm_shuffle_epi8(vt, reverseWords);
  auto elements = _mm_srli_epi16(words, 7);
  elements = _mm_and_si128(elements, _mm_set1_epi16(0x00ff));
  auto elements8 = _mm_packus_epi16(elements, _mm_setzero_si128());
  u32 selectWord = u32(source[0]) << 0 | u32(source[1]) << 8 | u32(source[2]) << 16 | u32(source[3]) << 24;
  auto select = _mm_cvtsi32_si128(selectWord);
  auto values = _mm_shuffle_epi8(elements8, select);
  auto destination = _mm_load_si128((const __m128i*)simdSFVDestination[base]);
  auto mask = _mm_load_si128((const __m128i*)simdSFVMask[base]);
  auto merged = _mm_andnot_si128(mask, _mm_loadu_si128((const __m128i*)target));
  merged = _mm_or_si128(merged, _mm_and_si128(mask, _mm_shuffle_epi8(values, destination)));
  _mm_storeu_si128((__m128i*)target, merged);
#else
  for(u32 offset = 0; offset < 4; offset++) {
    u32 code = source[offset];
    if(code < 8) target[(base + offset * 4) & 15] = vt.element(code) >> 7;
    else         target[(base + offset * 4) & 15] = 0;
  }
#endif
}

auto RSP::fastSHV(cr128& vt, u8* target, u32 code) -> void {
  u32 index = code & 15;
  auto source = simdSHVSource[code >> 4];
#if ARCHITECTURE_SUPPORTS_SSE4_1
  auto reverse = _mm_load_si128((const __m128i*)simdShuffleIdentity);
  auto vtb = _mm_shuffle_epi8(vt, reverse);
  auto sourceA = _mm_load_si128((const __m128i*)(source +  0));
  auto sourceB = _mm_load_si128((const __m128i*)(source + 16));
  auto bytesA = _mm_shuffle_epi8(vtb, sourceA);
  auto bytesB = _mm_shuffle_epi8(vtb, sourceB);
  auto wordsA = _mm_cvtepu8_epi16(bytesA);
  auto wordsB = _mm_cvtepu8_epi16(bytesB);
  auto words = _mm_or_si128(_mm_slli_epi16(wordsA, 1), _mm_srli_epi16(wordsB, 7));
  words = _mm_and_si128(words, _mm_set1_epi16(0x00ff));
  auto bytes = _mm_packus_epi16(words, _mm_setzero_si128());
  auto destination = _mm_load_si128((const __m128i*)simdSHVDestination[index]);
  auto mask = _mm_load_si128((const __m128i*)simdSHVMask[index]);
  auto merged = _mm_andnot_si128(mask, _mm_loadu_si128((const __m128i*)target));
  merged = _mm_or_si128(merged, _mm_and_si128(mask, _mm_shuffle_epi8(bytes, destination)));
  _mm_storeu_si128((__m128i*)target, merged);
#else
  for(u32 offset = 0; offset < 8; offset++) {
    u32 byteA = source[offset +  0];
    u32 byteB = source[offset + 16];
    u8 value = vt.byte(byteA) << 1 | vt.byte(byteB) >> 7;
    target[(index + offset * 2) & 15] = value;
  }
#endif
}

#undef IpuReg
#undef VuReg
#undef DmemReg
#undef ThreadReg
#undef PipelineReg
#undef BranchReg
#undef StatusReg
#undef R0
#undef Sa
#undef Rdn
#undef Rtn
#undef Rsn
#undef XRtn
#undef XRdn
#undef XCODE
#undef Vdn
#undef Vsn
#undef Vtn
#undef Rd
#undef Rt
#undef Rs
#undef XRd
#undef XRt
#undef Vd
#undef Vs
#undef Vt
#undef i16
#undef n16
#undef n26
#undef callvu

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
