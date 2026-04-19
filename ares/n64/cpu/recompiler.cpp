/*
CPU Recompiler: Architecture and Execution Model
================================================

Overview
--------
This file implements the N64 CPU JIT backend on top of nall::recompiler
(SLJIT). It translates guest instructions into host blocks that operate
directly on CPU/IPU/FPU/Pipeline state.

Main goals:
- keep the hot path inlined (minimal per-opcode overhead);
- preserve exact MIPS-visible behavior (delay slots, exceptions, timing);
- fall back to helpers/interpreter paths where complexity is still high.

Block lifecycle
---------------
1) Lookup:
   - block(vaddr, paddr, singleInstruction) probes a per-section cache:
     4 KiB logical sections, 1024 entry slots per section (word indexed).
   - each entry slot stores a linked list of Block keyed by stateKey.
   - lookup is O(1) to entry head + short linear scan on stateKey collisions.
2) Emit:
   - on miss, emit() builds host code with beginFunction()/endFunction().
3) Publish:
   - compiled code is inserted at head(entry) and later executed via Block::execute().

 Invalidation model
 ------------------
 - every memory write marks the touched section as dirty (hot path: one flag write).
 - section cleanup is lazy: the next lookup on that section clears its entry table.
 - Section metadata remains allocated and is reused; only block links are dropped.

Codegen model inside emit()
---------------------------
For each instruction in the block:
- decode metadata with decoderEXECUTEInfo() (OpInfo);
- load/advance virtual pipeline PC window (pc/nextpc/nstate);
- emit opcode body via emitEXECUTE()/emitSPECIAL()/...;
- account clocks in deferredCycles (flush only at synchronization boundaries);
- test EndBlock only when needed, then branch to epilogue;
- commit architectural state at explicit commit points.
- stop block at 4 KiB section boundary;
- stop block on opcodes marked JitStateKeyMayChange.

The hot loop avoids per-opcode generic helper calls when possible. Branch
families are emitted directly in JIT, including likely/link behavior and
delay-slot state transitions.

State and timing model
----------------------
- PipelineReg(pc/nextpc/state/nstate) holds transient control-flow state.
- Architectural commit writes pipeline state and ipu.pc only at controlled
  points (helper boundaries, branch boundaries, block termination).
- CPU::step() is emitted through deferred flushes, not per instruction.

JIT <-> interpreter interop
---------------------------
The dispatcher in cpu.cpp chooses JIT only when conditions allow it:
- dynamic recompiler enabled;
- instruction is in cacheable memory;
- fetch/devirtualization checks pass.

If JIT compilation cannot proceed (for example icache incoherence), emit()
returns nullptr and execution immediately falls back to interpreter decode for
that instruction stream.

Even inside JIT blocks, opcodes may still call C++ helpers (callf) for complex
or exceptional behavior. Those helpers share the same CPU state objects, so
exceptions/traps/faults naturally rejoin the normal interpreter-visible flow.

 State key and metrics
 ---------------------
 - computeStateKey() packs slow-changing CPU/FPU mode bits into an n64 key.
 - blocks are specialized by stateKey to remove avoidable runtime checks.
 - reportMetrics() prints coarse JIT health counters every fixed number of
   block() calls (hit/miss, list depth, emits, allocator flushes, etc.).
*/

auto CPU::Recompiler::computeStateKey() const -> u64 {
  n64 stateKey = 0;
  stateKey.bit( 0)     = self.scc.status.enable.coprocessor1;
  stateKey.bit( 1)     = self.scc.status.floatingPointMode;
  stateKey.bit( 2)     = self.scc.status.exceptionLevel;
  stateKey.bit( 3)     = self.scc.status.errorLevel;
  stateKey.bit( 4,  5) = self.scc.status.privilegeMode;
  stateKey.bit( 6)     = self.scc.status.userExtendedAddressing;
  stateKey.bit( 7)     = self.scc.status.supervisorExtendedAddressing;
  stateKey.bit( 8)     = self.scc.status.kernelExtendedAddressing;
  stateKey.bit( 9)     = self.scc.status.reverseEndian;
  stateKey.bit(10)     = self.scc.status.enable.coprocessor0;
  stateKey.bit(11, 12) = self.fpu.csr.roundMode;
  stateKey.bit(13)     = self.fpu.csr.flushSubnormals;
  stateKey.bit(14)     = self.fpu.csr.enable.inexact;
  stateKey.bit(15)     = self.fpu.csr.enable.underflow;
  stateKey.bit(16)     = self.fpu.csr.enable.overflow;
  stateKey.bit(17)     = self.fpu.csr.enable.divisionByZero;
  stateKey.bit(18)     = self.fpu.csr.enable.invalidOperation;
  return stateKey;
}

auto CPU::Recompiler::reportMetrics() -> void {
  auto calls = metrics.blockCalls;
  if(!calls) return;
  auto hitPermille = metrics.blockHits * 1000 / calls;
  auto avgLookupMilli = metrics.lookupSteps * 1000 / calls;
  print("CPU JIT metrics calls=", calls, " hit=", metrics.blockHits, " miss=", metrics.blockMisses,
        " hitpermille=", hitPermille, " lookupmilli=", avgLookupMilli,
        " lookupmax=", metrics.lookupStepsMax,
        " secalloc=", metrics.sectionAllocations, " secclear=", metrics.sectionDirtyClears,
        " secdrop=", metrics.sectionDirtyDrops, " emit=", metrics.emitCalls,
        " emitok=", metrics.emitSuccess, " emitabort=", metrics.emitAbortIcache,
        " flush=", metrics.allocatorFlushes,
        " linkcand=", metrics.linkCandidates,
        " linkdir=", metrics.linkInstalledDirect,
        " linkpend=", metrics.linkPendingQueued,
        " linkbp=", metrics.linkInstalledBackpatch,
        " linktaken=", metrics.linkTaken,
        " linkdirty=", metrics.linkAbortDirty,
        " linkbudget=", metrics.linkAbortBudget,
        " linkqueue=", metrics.linkAbortQueue,
        " linkmiss=", metrics.linkAbortNoTarget,
        " linkmissnocand=", metrics.linkAbortNoCandidate,
        " linkmissunresolved=", metrics.linkAbortNoResolvedTarget,
        " linkcandunsafe=", metrics.linkCandidateUnsafeDelaySlot,
        " nocandsec=", metrics.linkNoCandidateSectionBoundary,
        " nocandsingle=", metrics.linkNoCandidateSingleInstruction,
        " nocandstate=", metrics.linkNoCandidateStateKeyMayChange,
        " nocandcountcmp=", metrics.linkNoCandidateCountCompareWrite,
        " nocandnodirect=", metrics.linkNoCandidateNoDirectTarget,
        " nocandother=", metrics.linkNoCandidateOther, "\n");
}

auto CPU::Recompiler::section(u32 address) -> Section* {
  assert(isRdramAddress(address));
  if(!isRdramAddress(address)) return nullptr;
  auto index = sectionIndex(address);
  auto& section = sections[index];
  auto dirty = sectionDirty[index];
  if(!section) {
    metrics.sectionAllocations++;
    section = (Section*)allocator.acquire(sizeof(Section));
    memory::jitprotect(false);
    *section = {};
    memory::jitprotect(true);
    if(dirty) {
      metrics.sectionDirtyDrops++;
      sectionDirty[index] = 0;
    }
    return section;
  }
  if(dirty) {
    metrics.sectionDirtyClears++;
    memory::jitprotect(false);
    *section = {};
    memory::jitprotect(true);
    sectionDirty[index] = 0;
  }
  return section;
}

auto CPU::Recompiler::block(u64 vaddr, u32 address, bool singleInstruction) -> Block* {
  metrics.blockCalls++;
  if((metrics.blockCalls & (MetricsReportInterval - 1)) == 0) reportMetrics();

  auto section = this->section(address);
  if(!section) return nullptr;

  auto index = blockIndex(address);
  auto stateKey = computeStateKey();
  u64 lookupSteps = 0;
  for(auto block = section->blocks[index]; block; block = block->next) {
    lookupSteps++;
    if(block->stateKey == stateKey) {
      metrics.blockHits++;
      metrics.lookupSteps += lookupSteps;
      if(lookupSteps > metrics.lookupStepsMax) metrics.lookupStepsMax = lookupSteps;
      return block;
    }
  }
  metrics.blockMisses++;
  metrics.lookupSteps += lookupSteps;
  if(lookupSteps > metrics.lookupStepsMax) metrics.lookupStepsMax = lookupSteps;

  auto findLinked = [&](u32 targetAddress, u64 targetStateKey) -> Block* {
    auto targetIndex = blockIndex(targetAddress);
    for(auto target = section->blocks[targetIndex]; target; target = target->next) {
      if(target->stateKey != targetStateKey) continue;
      if(target->startAddress != targetAddress) continue;
      return target;
    }
    return nullptr;
  };

  auto resolvePending = [&](Block* target) -> void {
    auto targetIndex = blockIndex(target->startAddress);
    auto* pending = &section->pending[targetIndex];
    while(*pending) {
      auto entry = *pending;
      if(entry->expectedStateKey == target->stateKey && entry->expectedTargetAddress == target->startAddress) {
        entry->source->linkedBlock = target;
        metrics.linkInstalledBackpatch++;
        *pending = entry->next;
        continue;
      }
      pending = &entry->next;
    }
  };

  auto block = emit(vaddr, address, stateKey, singleInstruction);
  if(block) {
    block->next = section->blocks[index];
    section->blocks[index] = block;
    block->sectionDirty = sectionDirty.data() + sectionIndex(block->startAddress);
    if(block->linkAddress != ~0u) {
      metrics.linkCandidates++;
      if(auto target = findLinked(block->linkAddress, block->stateKey)) {
        block->linkedBlock = target;
        metrics.linkInstalledDirect++;
      } else {
        auto pending = (Pending*)allocator.acquire(sizeof(Pending));
        pending->source = block;
        pending->next = section->pending[blockIndex(block->linkAddress)];
        pending->expectedStateKey = block->stateKey;
        pending->expectedTargetAddress = block->linkAddress;
        section->pending[blockIndex(block->linkAddress)] = pending;
        metrics.linkPendingQueued++;
      }
    }
    resolvePending(block);
    memory::jitprotect(true);
  }
  return block;
}

#define IpuBase        offsetof(IPU, r[16])
#define IpuReg(r)      sreg(1), offsetof(IPU, r) - IpuBase
#define PipelineReg(x) mem(sreg(0), offsetof(CPU, pipeline) + offsetof(Pipeline, x))

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
auto CPU::Recompiler::emit(u64 vaddr, u32 address, u64 stateKey, bool singleInstruction) -> Block* {
  metrics.emitCalls++;
  if(unlikely(allocator.available() < 1_MiB)) {
    metrics.allocatorFlushes++;
    print("CPU allocator flush\n");
    allocator.release();
    reset();
  }

  // abort compilation of block asap if the instruction cache is not coherent
  if(!self.icache.coherent(vaddr, address)) {
    metrics.emitAbortIcache++;
    return nullptr;
  }

  beginFunction(3);

  u32 deferredCycles = 0;
  auto flushDeferredCycles = [&](u32 clocks) -> void {
    if(!clocks) return;
    callf(&CPU::step, imm(clocks));
  };
  auto flushDeferred = [&]() -> void {
    flushDeferredCycles(deferredCycles);
    deferredCycles = 0;
  };
  auto commitArchitecturalState = [&]() -> void {
    mov32(PipelineReg(state), PipelineReg(nstate));
    mov64(mem(IpuReg(pc)), PipelineReg(pc));
  };

  Thread thread;
  u32 startAddress = address;
  u32 startSection = sectionIndex(address);
  bool hasBranched = 0;
  u32 lastBranchLinkAddress = ~0u;
  u32 linkAddress = ~0u;
  u8 noCandidateReason = Block::NoCandidateNone;
  int numInsn = 0;
  constexpr u32 branchToSelf = 0x1000'ffff;  //beq 0,0,<pc>
  u32 jumpToSelf = 2 << 26 | vaddr >> 2 & 0x3ff'ffff;  //j <pc>
  auto writesCountCompare = [](u32 instruction) -> bool {
    if(instruction >> 26 != 0x10) return false;
    auto op = instruction >> 21 & 0x1f;
    if(op != 0x04 && op != 0x05) return false;
    auto rd = instruction >> 11 & 31;
    return rd == 9 || rd == 11;
  };
  auto linkAddressFromVaddr = [&](u64 targetVaddr) -> u32 {
    auto access = self.devirtualize<Read, Word>(targetVaddr, false, false);
    if(!access || !access.cache) return ~0u;
    if(sectionIndex(access.paddr) != startSection) return ~0u;
    return access.paddr;
  };
  auto directBranchLinkAddress = [&](u64 branchVaddr, u32 instruction) -> u32 {
    auto opcode = instruction >> 26;
    auto rs = instruction >> 21 & 31;
    auto rt = instruction >> 16 & 31;
    auto branchTargetVaddr = branchVaddr + 4 + (s64(s16(instruction)) << 2);
    auto fallthroughVaddr = branchVaddr + 8;
    if(opcode == 0x02 || opcode == 0x03) {
      auto targetVaddr = (branchVaddr & 0xffff'ffff'f000'0000ull) | (u64(instruction & 0x03ff'ffff) << 2);
      return linkAddressFromVaddr(targetVaddr);
    }
    if(opcode == 0x04 || opcode == 0x14) {
      if(rs != rt) return ~0u;
      return linkAddressFromVaddr(branchTargetVaddr);
    }
    if(opcode == 0x05 || opcode == 0x15) {
      if(rs != rt) return ~0u;
      return linkAddressFromVaddr(fallthroughVaddr);
    }
    if(opcode == 0x06 || opcode == 0x16) {
      if(rs != 0) return ~0u;
      return linkAddressFromVaddr(branchTargetVaddr);
    }
    if(opcode == 0x07 || opcode == 0x17) {
      if(rs != 0) return ~0u;
      return linkAddressFromVaddr(fallthroughVaddr);
    }
    if(opcode != 0x01) return ~0u;
    if(rs != 0) return ~0u;
    if(rt == 0x00 || rt == 0x02 || rt == 0x10 || rt == 0x12) {
      return linkAddressFromVaddr(fallthroughVaddr);
    }
    if(rt == 0x01 || rt == 0x03 || rt == 0x11 || rt == 0x13) {
      return linkAddressFromVaddr(branchTargetVaddr);
    }
    return ~0u;
  };
  while(true) {
    u32 instruction = bus.read<Word>(address, thread, RBusDevice::ARES_JIT);
    OpInfo info = self.decoderEXECUTEInfo(instruction);
    mov32(PipelineReg(nstate), imm(0));
    mov64(reg(0), PipelineReg(nextpc));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    if(callInstructionPrologue) {
      flushDeferred();
      callf(&CPU::instructionPrologue, imm64(vaddr), imm(instruction));
    }
    if(numInsn == 0 || (vaddr&0x1f)==0){
      flushDeferred();
      //abort compilation of block if the instruction cache is not coherent
      if(!self.icache.coherent(vaddr, address)) {
        metrics.emitAbortIcache++;
        resetCompiler();
        return nullptr;
      }
      callf(&CPU::jitFetch, imm64(vaddr), imm(address));
    }
    if(info.jitMustFlushBeforeCall() || info.jitAddsExtraCyclesInternally()) {
      flushDeferred();
    }
    numInsn++;
    bool branched = emitEXECUTE(instruction);
    u32 branchLinkAddress = ~0u;
    if(branched) branchLinkAddress = directBranchLinkAddress(vaddr, instruction);
    if(unlikely(instruction == branchToSelf || instruction == jumpToSelf)) {
      deferredCycles += 64 * 2;
    } else {
      deferredCycles += 1 * 2;
    }
    flushDeferred();
    test32(PipelineReg(state), imm(Pipeline::EndBlock), set_z);
    mov32(PipelineReg(state), PipelineReg(nstate));
    mov64(mem(IpuReg(pc)), PipelineReg(pc));

    vaddr += 4;
    address += 4;
    jumpToSelf += 4;
    bool countCompareWrite = writesCountCompare(instruction);
    bool sectionBoundary = sectionIndex(address) != startSection;
    bool terminal = hasBranched || sectionBoundary || singleInstruction;
    terminal = terminal || info.jitStateKeyMayChange() || countCompareWrite;
    bool commitNow = info.jitMayCallf() || branched || terminal;
    if(commitNow) commitArchitecturalState();
    bool safeDelaySlotLink = !info.jitMayCallf() && !info.mayException() && !info.mayFault();
    bool delaySlotLinkEligible = terminal && hasBranched && !singleInstruction
    && !info.jitStateKeyMayChange() && !countCompareWrite;
    if(delaySlotLinkEligible && safeDelaySlotLink) {
      linkAddress = lastBranchLinkAddress;
    }
    if(delaySlotLinkEligible && !safeDelaySlotLink && lastBranchLinkAddress != ~0u) {
      metrics.linkCandidateUnsafeDelaySlot++;
    }
    if(terminal) {
      if(!hasBranched) jumpEpilog(flag_nz);
      if(linkAddress != ~0u) {
        noCandidateReason = Block::NoCandidateNone;
      } else if(singleInstruction) {
        noCandidateReason = Block::NoCandidateSingleInstruction;
      } else if(info.jitStateKeyMayChange()) {
        noCandidateReason = Block::NoCandidateStateKeyMayChange;
      } else if(countCompareWrite) {
        noCandidateReason = Block::NoCandidateCountCompareWrite;
      } else if(hasBranched) {
        noCandidateReason = Block::NoCandidateNoDirectTarget;
      } else if(sectionBoundary) {
        noCandidateReason = Block::NoCandidateSectionBoundary;
      } else {
        noCandidateReason = Block::NoCandidateOther;
      }
      break;
    }
    hasBranched = branched;
    lastBranchLinkAddress = branchLinkAddress;
    jumpEpilog(flag_nz);
  }

  flushDeferred();
  callf(&CPU::jitLinkedCode);
  sljit_set_label(sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_RETURN_REG, 0, SLJIT_IMM, 0), epilogue);
  mov64(reg(3), reg(0));
  mov64(reg(0), sreg(0));
  mov64(reg(1), sreg(1));
  mov64(reg(2), sreg(2));
  sljit_s32 linkArgs = SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1)
                     | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2)
                     | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 3);
  sljit_emit_icall(compiler, SLJIT_CALL | SLJIT_CALL_RETURN, linkArgs, reg(3).fst, reg(3).snd);
  jumpEpilog();

  memory::jitprotect(false);
  auto block = (Block*)allocator.acquire(sizeof(Block));
  block->code = endFunction();
  block->next = nullptr;
  block->linkedBlock = nullptr;
  block->stateKey = stateKey;
  block->startAddress = startAddress;
  block->endAddress = address;
  block->linkAddress = linkAddress;
  block->sectionDirty = sectionDirty.data() + startSection;
  block->noCandidateReason = noCandidateReason;
  metrics.emitSuccess++;

//print(hex(PC, 8L), " ", instructions, " ", size(), "\n");
  return block;
}
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

#define FpuBase   offsetof(FPU, r[16])
#define FpuReg(r) sreg(2), offsetof(FPU, r) - FpuBase
#define Fd        FpuReg(r[0]) + Fdn * sizeof(r64)
#define Fs        FpuReg(r[0]) + Fsn * sizeof(r64)
#define Ft        FpuReg(r[0]) + Ftn * sizeof(r64)

#define XRd       IpuReg(r[0]) + XRdn * sizeof(r64)
#define XRt       IpuReg(r[0]) + XRtn * sizeof(r64)

#define i16 s16(instruction)
#define n16 u16(instruction)
#define n26 u32(instruction & 0x03ff'ffff)

auto CPU::Recompiler::emitZeroClear(u32 n) -> void {
  if(n == 0) mov64(mem(IpuReg(r[0])), imm(0));
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
    and64(reg(0), PipelineReg(pc), imm(0xffff'ffff'f000'0000ull));
    or64(reg(0), reg(0), imm(n26 << 2));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    return 1;
  }

  //JAL n26
  case 0x03: {
    add64(reg(1), PipelineReg(pc), imm(4));
    mov64(mem(IpuReg(r[31])), reg(1));
    and64(reg(0), PipelineReg(pc), imm(0xffff'ffff'f000'0000ull));
    or64(reg(0), reg(0), imm(n26 << 2));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    return 1;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    cmp64(mem(Rs), mem(Rt), set_z);
    auto notTaken = jump(flag_nz);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    return 1;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    cmp64(mem(Rs), mem(Rt), set_z);
    auto taken = jump(flag_nz);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BLEZ Rs,i16
  case 0x06: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto notTaken = jump(flag_sgt);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    return 1;
  }

  //BGTZ Rs,i16
  case 0x07: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto taken = jump(flag_sgt);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
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
    cmp64(mem(Rs), mem(Rt), set_z);
    auto taken = jump(flag_z);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BNEL Rs,Rt,i16
  case 0x15: {
    cmp64(mem(Rs), mem(Rt), set_z);
    auto notTaken = jump(flag_z);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BLEZL Rs,i16
  case 0x16: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto notTaken = jump(flag_sgt);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BGTZL Rs,i16
  case 0x17: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto taken = jump(flag_sgt);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //DADDI Rt,Rs,i16
  case 0x18: {
    callf(&CPU::DADDI, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //DADDIU Rt,Rs,i16
  case 0x19: {
    callf(&CPU::DADDIU, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
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
    mov64(PipelineReg(nextpc), mem(Rs));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    return 1;
  }

  //JALR Rd,Rs
  case 0x09: {
    mov64(reg(1), mem(Rs));
    add64(reg(0), PipelineReg(pc), imm(4));
    if(Rdn) mov64(mem(Rd), reg(0));
    mov64(PipelineReg(nextpc), reg(1));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
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
    callf(&CPU::DSLLV, mem(Rd), mem(Rt), mem(Rs));
    emitZeroClear(Rdn);
    return 0;
  }

  //INVALID
  case 0x15: {
    callf(&CPU::INVALID);
    return 1;
  }

  //DSRLV Rd,Rt,Rs
  case 0x16: {
    callf(&CPU::DSRLV, mem(Rd), mem(Rt), mem(Rs));
    emitZeroClear(Rdn);
    return 0;
  }

  //DSRAV Rd,Rt,Rs
  case 0x17: {
    callf(&CPU::DSRAV, mem(Rd), mem(Rt), mem(Rs));
    emitZeroClear(Rdn);
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
    callf(&CPU::DADD, mem(Rd), mem(Rs), mem(Rt));
    emitZeroClear(Rdn);
    return 0;
  }

  //DADDU Rd,Rs,Rt
  case 0x2d: {
    callf(&CPU::DADDU, mem(Rd), mem(Rs), mem(Rt));
    emitZeroClear(Rdn);
    return 0;
  }

  //DSUB Rd,Rs,Rt
  case 0x2e: {
    callf(&CPU::DSUB, mem(Rd), mem(Rs), mem(Rt));
    emitZeroClear(Rdn);
    return 0;
  }

  //DSUBU Rd,Rs,Rt
  case 0x2f: {
    callf(&CPU::DSUBU, mem(Rd), mem(Rs), mem(Rt));
    emitZeroClear(Rdn);
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
    callf(&CPU::DSLL, mem(Rd), mem(Rt), imm(Sa));
    emitZeroClear(Rdn);
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
    callf(&CPU::DSRA, mem(Rd), mem(Rt), imm(Sa));
    emitZeroClear(Rdn);
    return 0;
  }

  //DSLL32 Rd,Rt,Sa
  case 0x3c: {
    callf(&CPU::DSLL, mem(Rd), mem(Rt), imm(Sa+32));
    emitZeroClear(Rdn);
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
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BGEZ Rs,i16
  case 0x01: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    return 1;
  }

  //BLTZL Rs,i16
  case 0x02: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BGEZL Rs,i16
  case 0x03: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    setLabel(done);
    return 1;
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
    add32(reg(0), PipelineReg(pc), imm(4));
    mov64_s32(reg(0), reg(0));
    mov64(mem(IpuReg(r[31])), reg(0));
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    add32(reg(0), PipelineReg(pc), imm(4));
    mov64_s32(reg(0), reg(0));
    mov64(mem(IpuReg(r[31])), reg(0));
    return 1;
  }

  //BLTZALL Rs,i16
  case 0x12: {
    add32(reg(0), PipelineReg(pc), imm(4));
    mov64_s32(reg(0), reg(0));
    mov64(mem(IpuReg(r[31])), reg(0));
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //BGEZALL Rs,i16
  case 0x13: {
    add32(reg(0), PipelineReg(pc), imm(4));
    mov64_s32(reg(0), reg(0));
    mov64(mem(IpuReg(r[31])), reg(0));
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    add64(reg(0), PipelineReg(pc), imm(4));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    setLabel(done);
    return 1;
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

  //XDETECT
  case 0x20: {
    callf(&CPU::XDETECT, mem(XRd), imm(XCODE));
    return 0;
  }

  //XLOG
  case 0x25: {
    callf(&CPU::XLOG, mem(XRd), mem(XRt), imm(XCODE));
    return 0;
  }

  //XHEXDUMP
  case 0x27: {
    callf(&CPU::XHEXDUMP, mem(XRd), mem(XRt));
    return 0;
  }

  //XPROF
  case 0x28: {
    callf(&CPU::XPROF, mem(XRd), imm(XCODE));
    return 0;
  }

  //XPROFREAD
  case 0x29: {
    callf(&CPU::XPROFREAD, mem(XRd), mem(XRt));
    return 0;
  }

  //XEXCEPTION
  case 0x2a: {
    callf(&CPU::XEXCEPTION, mem(XRt));
    return 0;
  }

  //XIOCTL
  case 0x2c: {
    callf(&CPU::XIOCTL, imm(XCODE));
    return 0;
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
#undef FpuBase
#undef FpuReg
#undef Fd
#undef Fs
#undef Ft
#undef XRd
#undef XRt
#undef i16
#undef n16
#undef n26

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
