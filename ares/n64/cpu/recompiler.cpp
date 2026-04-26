/*
CPU Recompiler: Architecture and Optimization Notes
===================================================

Overview
--------
This file implements the N64 CPU JIT backend on top of nall::recompiler
(SLJIT). It translates guest instructions into host blocks that operate
directly on CPU/IPU/FPU/Pipeline state.

Goals:
- keep the common path close to pure opcode semantics;
- preserve MIPS-visible behavior (delay slots, exceptions, timing);
- keep uncommon behavior in helper calls or guarded slow paths.

State key specialization
------------------------
- computeStateKey() packs slow-changing CPU/FPU mode bits into a compact key.
- block cache entries are specialized by stateKey to avoid repeated runtime
  checks in compiled code.

Block lifecycle
---------------
1) Lookup
   - block(vaddr, paddr, singleInstruction) probes a per-section cache.
   - sections are 4 KiB; each section has 1024 word-indexed entry slots.
   - each entry slot stores a linked list of Block keyed by stateKey.
2) Emit
   - on cache miss, emit() builds host code with beginFunction()/endFunction().
3) Publish
   - compiled block is inserted at entry head and later executed by
     Block::execute().

Execution model inside emit()
-----------------------------
For each instruction in a block, the emitter:
- decodes metadata with decoderEXECUTEInfo() (OpInfo);
- updates virtual pipeline window (pc/nextpc/state/nstate) only when needed;
- emits opcode body via emitEXECUTE()/emitSPECIAL()/...;
- accumulates cycles and nextpc advances in deferred form;
- synchronizes state/timing only at explicit boundaries;
- stops block at section boundary or on JitStateKeyMayChange.

Hot-loop optimizations
----------------------
1) Deferred cycle accounting
   - per-opcode cycles are accumulated in deferredCycles;
   - emitCpuStep / deferred flush applies Thread::clock only at synchronization
     boundaries (helper/branch/terminal transitions).

2) Virtual PC with selective architectural commit
   - ipu.pc is not written on every opcode;
   - commit happens only at correctness boundaries:
     block entry, branch/helper opcodes, and terminal paths.

3) Eager pc/nextpc materialization
   - each opcode materializes pipeline pc/nextpc directly in the main emit loop.

4) Pipeline state machinery
   - nstate clear, EndBlock test, state<-nstate commit, and
     jumpEpilog(flag_nz) are emitted in the main loop.

5) Helper-call PC rematerialization
   - in linear non-delay-slot helper paths, ipu.pc is rematerialized from the
     compile-time vaddr immediate;
   - delay-slot/branch-sensitive cases still use runtime pipeline state to
     preserve precise exception PC behavior.

Block linking model
-------------------
Goal:
- reduce dispatcher round-trips by chaining compatible blocks directly.

How linking is built:
- only intra-section edges are considered;
- at branch termination, emit() may record taken and not-taken successors;
- links are disabled on boundaries that may change execution mode
  (single-instruction mode, state-key-changing ops, SCC count/compare writes);
- delay-slot linking is gated by safety (no helper call, exception, or fault
  risk in delay-slot opcode).

How linking is resolved:
- each section maintains pending predecessor lists by target word index;
- publish attempts direct target resolution first (stateKey + startAddress);
- unresolved edges are queued and backpatched when the target is later emitted;
- if taken/not-taken collapse to same target, only one pending lookup is used.

Runtime link gate (jitLinkedCode):
- reject links on dirty sections;
- reject links when clock/queue budget says return to dispatcher;
- choose taken/not-taken edge from final architectural PC;
- chain only if resolved, otherwise return to epilogue.

Invalidation model
------------------
- memory writes mark the touched section dirty (hot path: one flag write);
- cleanup is lazy: next lookup on that section clears its entry table;
- section metadata is reused, while block/link contents are dropped.

JIT <-> interpreter interop
---------------------------
The dispatcher (cpu.cpp) enters JIT only when dynamic recompiler mode,
cacheability, and fetch/devirtualization preconditions are satisfied.

emit() may still return nullptr (for example icache incoherence), in which case
execution immediately falls back to interpreter decode for that stream.

Even inside JIT blocks, complex/exceptional opcodes may call C++ helpers.
Helpers share the same CPU state objects, so exceptions/traps/faults naturally
rejoin interpreter-visible control flow.
*/

auto CPU::Recompiler::computeStateKey() const -> u64 {
  StateKey stateKey = 0;
  stateKey.setCoprocessor1Enabled(self.scc.status.enable.coprocessor1);
  stateKey.setFloatingPointMode(self.scc.status.floatingPointMode);
  stateKey.setExceptionLevel(self.scc.status.exceptionLevel);
  stateKey.setErrorLevel(self.scc.status.errorLevel);
  stateKey.setPrivilegeMode(self.scc.status.privilegeMode);
  stateKey.setUserExtendedAddressing(self.scc.status.userExtendedAddressing);
  stateKey.setSupervisorExtendedAddressing(self.scc.status.supervisorExtendedAddressing);
  stateKey.setKernelExtendedAddressing(self.scc.status.kernelExtendedAddressing);
  stateKey.setReverseEndian(self.scc.status.reverseEndian);
  stateKey.setCoprocessor0Enabled(self.scc.status.enable.coprocessor0);
  stateKey.setFpuRoundMode(self.fpu.csr.roundMode);
  stateKey.setFpuFlushSubnormals(self.fpu.csr.flushSubnormals);
  stateKey.setFpuInexactEnabled(self.fpu.csr.enable.inexact);
  stateKey.setFpuUnderflowEnabled(self.fpu.csr.enable.underflow);
  stateKey.setFpuOverflowEnabled(self.fpu.csr.enable.overflow);
  stateKey.setFpuDivisionByZeroEnabled(self.fpu.csr.enable.divisionByZero);
  stateKey.setFpuInvalidOperationEnabled(self.fpu.csr.enable.invalidOperation);
  const u64 cachedBase = 0xffff'ffff'8000'0000ull;
  const u64 cachedEnd  = 0xffff'ffff'807f'ffffull;
  auto gp = self.ipu.r[28].u64;
  auto sp = self.ipu.r[29].u64;
  bool gpCached = gp >= cachedBase && gp <= cachedEnd;
  stateKey.setGpCachedRdram(gpCached);
  stateKey.setGpCachedRdramOff16(gp >= cachedBase + 0x8000 && gp <= cachedEnd - 0x7fff);
  stateKey.setGpAligned4((gp & 3) == 0);
  stateKey.setGpAligned8((gp & 7) == 0);
  stateKey.setSpAligned4((sp & 3) == 0);
  stateKey.setSpAligned8((sp & 7) == 0);
  return stateKey;
}

auto CPU::Recompiler::reservedInstruction64() const -> bool {
  if(emitStateKey.exceptionLevel() || emitStateKey.errorLevel()) return 0;
  auto privilegeMode = emitStateKey.privilegeMode();
  if(privilegeMode == 1) return !emitStateKey.supervisorExtendedAddressing();
  if(privilegeMode >= 2) return !emitStateKey.userExtendedAddressing();
  return 0;
}

auto CPU::Recompiler::updateStackPointerStateKey(s16 offset) -> void {
  bool oldAligned4 = emitStateKey.spAligned4();
  bool oldAligned8 = emitStateKey.spAligned8();
  bool newAligned4 = oldAligned4 && (offset & 3) == 0;
  bool newAligned8 = oldAligned8 && (offset & 7) == 0;
  emitStateKey.setSpAligned4(newAligned4);
  emitStateKey.setSpAligned8(newAligned8);
  emitStateKeyChanged = emitStateKeyChanged || oldAligned4 != newAligned4 || oldAligned8 != newAligned8;
}

auto CPU::Recompiler::section(u32 address) -> Section* {
  assert(isRdramAddress(address));
  if(!isRdramAddress(address)) return nullptr;
  auto index = sectionIndex(address);
  auto& section = sections[index];
  auto dirty = sectionDirty[index];
  if(!section) {
    section = (Section*)allocator.acquire(sizeof(Section));
    memory::jitprotect(false);
    *section = {};
    memory::jitprotect(true);
    if(dirty) {
      sectionDirty[index] = 0;
    }
    return section;
  }
  if(dirty) {
    memory::jitprotect(false);
    *section = {};
    memory::jitprotect(true);
    sectionDirty[index] = 0;
  }
  return section;
}

auto CPU::Recompiler::block(u64 vaddr, u32 address, bool singleInstruction) -> Block* {
  auto section = this->section(address);
  if(!section) return nullptr;

  auto index = blockIndex(address);
  auto stateKey = computeStateKey();
  for(auto block = section->blocks[index]; block; block = block->next) {
    if(block->stateKey == stateKey) {
      return block;
    }
  }

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
        if(entry->source->linkAddressTaken == target->startAddress) entry->source->linkedBlockTaken = target;
        if(entry->source->linkAddressNotTaken == target->startAddress) entry->source->linkedBlockNotTaken = target;
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
    bool hasTaken = block->linkAddressTaken != ~0u;
    bool hasNotTaken = block->linkAddressNotTaken != ~0u;
    auto linkTarget = [&](u32 targetAddress) -> Block* {
      auto target = findLinked(targetAddress, block->stateKey);
      if(target) return target;
      auto pending = (Pending*)allocator.acquire(sizeof(Pending));
      pending->source = block;
      pending->next = section->pending[blockIndex(targetAddress)];
      pending->expectedStateKey = block->stateKey;
      pending->expectedTargetAddress = targetAddress;
      section->pending[blockIndex(targetAddress)] = pending;
      return nullptr;
    };
    if(hasTaken || hasNotTaken) {
      if(hasTaken) block->linkedBlockTaken = linkTarget(block->linkAddressTaken);
      if(hasNotTaken && block->linkAddressNotTaken == block->linkAddressTaken) {
        block->linkedBlockNotTaken = block->linkedBlockTaken;
      } else if(hasNotTaken) {
        block->linkedBlockNotTaken = linkTarget(block->linkAddressNotTaken);
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
#define CpuIcacheLineBytes sizeof(CPU::InstructionCache::Line)
#define CpuIcacheTagKey0Off offsetof(CPU, icache.lines[0].tagKey)
#define IcacheTagKeyMem(lineIndex) \
  mem(sreg(0), sljit_sw(CpuIcacheTagKey0Off) + sljit_sw(lineIndex) * sljit_sw(CpuIcacheLineBytes))
#define CpuProfileIcacheHitsOff (offsetof(CPU, profile) + offsetof(CPU::Profile, icacheHits))
#define ProfileIcacheHitsMem mem(sreg(0), CpuProfileIcacheHitsOff)
#define CpuProfileIcacheMissesOff (offsetof(CPU, profile) + offsetof(CPU::Profile, icacheMisses))
#define ProfileIcacheMissesMem mem(sreg(0), CpuProfileIcacheMissesOff)
#define RdramRbusIcacheReadsAddr \
  ((sljit_sw)(uintptr_t)&rdram.profile.metrics[(u32)RBusDevice::VR4300_ICACHE].reads)
#define CpuClockMem mem(sreg(0), offsetof(CPU, clock))
#define CpuIcacheWords0Off offsetof(CPU, icache.lines[0].words[0])
#define IcacheLineWordsMem(lineIndex, byteOff) \
  mem(sreg(0), sljit_sw(CpuIcacheWords0Off + CpuIcacheLineBytes * (lineIndex) + (byteOff)))
#define CpuDcacheLineBytes sizeof(CPU::DataCache::Line)
#define CpuDcacheLine0Off offsetof(CPU, dcache.lines[0])
#define DcacheLineTagKeyOff offsetof(CPU::DataCache::Line, tagKey)
#define DcacheLineDirtyOff offsetof(CPU::DataCache::Line, dirty)
#define DcacheLineDirtyPcOff offsetof(CPU::DataCache::Line, dirtyPc)
#define DcacheLineWordsOff offsetof(CPU::DataCache::Line, words[0])
#define CpuProfileDcacheHitsOff (offsetof(CPU, profile) + offsetof(CPU::Profile, dcacheHits))
#define ProfileDcacheHitsMem mem(sreg(0), CpuProfileDcacheHitsOff)

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
auto CPU::Recompiler::emit(u64 vaddr, u32 address, u64 stateKey, bool singleInstruction) -> Block* {
  emitStateKey = stateKey;
  emitSingleInstruction = singleInstruction;
  emitStateKeyChanged = false;
  if(unlikely(allocator.available() < 1_MiB)) {
    print("CPU allocator flush\n");
    allocator.release();
    reset();
  }

  // abort compilation if outside of RAM or not cache-coherent
  const u32 icacheTag = address & ~0xfffu;
  const u32 icacheBurstHi = icacheTag + 0xfe0u;
  const bool icacheBurstOk = 
    icacheBurstHi <= 0x07ff'ffffu
    || (Model::Aleck64() && icacheTag > 0xbfff'ffffu && icacheBurstHi <= 0xc07f'ffffu);
  if(!icacheBurstOk) return nullptr;

  if(!self.icache.coherent(vaddr, address)) {
    return nullptr;
  }

  beginFunction(3);
  slowPaths.clear();
  emitDeferredCycles = 0;

  Thread thread;
  u32 startAddress = address;
  u32 startSection = sectionIndex(address);
  bool hasBranched = 0;
  int numInsn = 0;
  constexpr u32 branchToSelf = 0x1000'ffff;  //beq 0,0,<pc>
  u32 jumpToSelf = 2 << 26 | vaddr >> 2 & 0x3ff'ffff;  //j <pc>
  struct BranchLinks {
    u64 takenVaddr = ~0ull;
    u64 notTakenVaddr = ~0ull;
    u32 takenAddress = ~0u;
    u32 notTakenAddress = ~0u;
  };
  auto writesCountCompare = [](u32 instruction) -> bool {
    if(instruction >> 26 != 0x10) return false;
    auto op = instruction >> 21 & 0x1f;
    if(op != 0x04 && op != 0x05) return false;
    auto rd = instruction >> 11 & 31;
    return rd == 9 || rd == 11;
  };
  auto fillLinkFromVaddr = [&](BranchLinks& links, bool taken, u64 targetVaddr) -> void {
    auto access = self.devirtualize<Read, Word>(targetVaddr, false, false);
    if(!access) return;
    if(!access.cache) return;
    if(sectionIndex(access.paddr) != startSection) return;
    if(taken) {
      links.takenAddress = access.paddr;
      links.takenVaddr = targetVaddr;
      return;
    }
    links.notTakenAddress = access.paddr;
    links.notTakenVaddr = targetVaddr;
  };
  auto directBranchLinkAddress = [&](u64 branchVaddr, u32 instruction) -> BranchLinks {
    BranchLinks links;
    auto opcode = instruction >> 26;
    auto rt = instruction >> 16 & 31;
    auto branchTargetVaddr = branchVaddr + 4 + (s64(s16(instruction)) << 2);
    auto fallthroughVaddr = branchVaddr + 8;
    if(opcode == 0x02 || opcode == 0x03) {
      auto targetVaddr = (branchVaddr & 0xffff'ffff'f000'0000ull) | (u64(instruction & 0x03ff'ffff) << 2);
      fillLinkFromVaddr(links, true, targetVaddr);
      return links;
    }
    if(opcode == 0x04 || opcode == 0x05 || opcode == 0x06 || opcode == 0x07
    || opcode == 0x14 || opcode == 0x15 || opcode == 0x16 || opcode == 0x17) {
      fillLinkFromVaddr(links, true, branchTargetVaddr);
      fillLinkFromVaddr(links, false, fallthroughVaddr);
      return links;
    }
    if(opcode == 0x01) {
      if(rt == 0x00 || rt == 0x01 || rt == 0x02 || rt == 0x03
      || rt == 0x10 || rt == 0x11 || rt == 0x12 || rt == 0x13) {
        fillLinkFromVaddr(links, true, branchTargetVaddr);
        fillLinkFromVaddr(links, false, fallthroughVaddr);
      }
      return links;
    }
    if(opcode == 0x11 && (instruction >> 21 & 31) == 0x08) {
      if(!emitStateKey.coprocessor1Enabled()) return links;
      fillLinkFromVaddr(links, true, branchTargetVaddr);
      fillLinkFromVaddr(links, false, fallthroughVaddr);
      return links;
    }
    return links;
  };
  BranchLinks links;
  bool branchLinksValid = false;
  auto bindSlowPaths = [&](size_t first, sljit_label* resume, u32 deferredCycles, bool jumpEpilog) -> void {
    for(auto n = first; n < slowPaths.size(); n++) {
      auto& slow = slowPaths[n];
      if(slow.icacheMiss) continue;
      slow.resume = resume;
      slow.instructionCycles = deferredCycles - slow.deferredCycles;
      slow.jumpEpilog = jumpEpilog;
    }
  };
  while(true) {
    u32 instruction = bus.read<Word>(address, thread, RBusDevice::ARES_JIT);
    OpInfo info = self.decoderEXECUTEInfo(instruction);
    emitVaddr = vaddr;
    emitCallfSetupDone = false;
    emitCallfEmitted = false;
    bool countCompareWrite = writesCountCompare(instruction);
    bool sectionBoundary = sectionIndex(address + 4) != startSection;
    bool terminal = hasBranched || sectionBoundary || singleInstruction;
    terminal = terminal || info.jitStateKeyMayChange() || countCompareWrite;
    mov32(PipelineReg(nstate), imm(0));
    mov64(reg(0), PipelineReg(nextpc));
    mov64(PipelineReg(pc), reg(0));
    add64(PipelineReg(nextpc), reg(0), imm(4));
    if(callInstructionPrologue) {
      flushDeferredCycles();
      callf(&CPU::instructionPrologue, imm64(vaddr), imm(instruction));
    }
    if(numInsn == 0 || (vaddr&0x1f)==0){
      flushDeferredCycles();
      if(!self.icache.coherent(vaddr, address)) {
        resetCompiler();
        return nullptr;
      }
      const u32 lineIndex = u32(vaddr >> 5) & 0x1ffu;
      const u32 expectedTagKey = (address & ~0xfffu) | 1u;
      cmp32(IcacheTagKeyMem(lineIndex), imm(expectedTagKey), set_z);
      auto icacheMiss = jump(flag_ne);
      if(system.homebrewMode) {
        add64(ProfileIcacheHitsMem, ProfileIcacheHitsMem, imm(1));
      }
      deferSlowPathCacheMiss(icacheMiss, address);
    }
    auto slowPathStart = slowPaths.size();
    numInsn++;
    bool branched = emitEXECUTE(instruction, false);
    if(branched) links = directBranchLinkAddress(vaddr, instruction);
    u32 instructionCycles = 1 * 2;
    if(unlikely(instruction == branchToSelf || instruction == jumpToSelf)) {
      instructionCycles = 64 * 2;
    }
    emitDeferredCycles += instructionCycles;
    if(hasBranched || info.branch() || emitCallfEmitted) flushDeferredCycles();
    test32(PipelineReg(state), imm(Pipeline::EndBlock), set_z);
    mov32(PipelineReg(state), PipelineReg(nstate));
    mov64(mem(IpuReg(pc)), PipelineReg(pc));

    vaddr += 4;
    address += 4;
    jumpToSelf += 4;
    bool safeDelaySlotLink = false;
    bool delaySlotLinkEligible = terminal && hasBranched && !singleInstruction
    && !info.jitStateKeyMayChange() && !countCompareWrite && !emitStateKeyChanged;
    if(delaySlotLinkEligible && safeDelaySlotLink) branchLinksValid = true;
    if(terminal) {
      if(!hasBranched) jumpEpilog(flag_nz);
      if(slowPaths.size() != slowPathStart) {
        auto resume = sljit_emit_label(compiler);
        bindSlowPaths(slowPathStart, resume, emitDeferredCycles, !hasBranched);
      }
      break;
    }
    hasBranched = branched;
    jumpEpilog(flag_nz);
    if(slowPaths.size() != slowPathStart) {
      auto resume = sljit_emit_label(compiler);
      bindSlowPaths(slowPathStart, resume, emitDeferredCycles, true);
    }
  }

  flushDeferredCycles();
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
  for(auto& slow : slowPaths) {
    auto enter = sljit_emit_label(compiler);
    for(auto jump : slow.enters) sljit_set_label(jump, enter);
    emitVaddr = slow.vaddr;
    emitDeferredCycles = slow.deferredCycles;
    emitCallfSetupDone = false;
    emitCallfEmitted = false;
    if(slow.icacheMiss) {
      const u32 lineIndex = u32(slow.vaddr >> 5) & 0x1ffu;
      const u32 burst = (slow.icachePaddr & ~0xfffu) | ((lineIndex << 5) & 0xfe0u);
      const u32 tagKey = (slow.icachePaddr & ~0xfffu) | 1u;
      const bool sdram = Model::Aleck64() && burst > 0xbfff'ffffu;
      const sljit_sw ramDataField = sdram ? (sljit_sw)(uintptr_t)&aleck64.sdram.data
                                           : (sljit_sw)(uintptr_t)&rdram.ram.data;
      const u32 ramByteOff = sdram ? (burst & 0xffffffu) : burst;
      if(system.homebrewMode) {
        add64(ProfileIcacheMissesMem, ProfileIcacheMissesMem, imm(1));
        if(!sdram) add64(mem0(RdramRbusIcacheReadsAddr), mem0(RdramRbusIcacheReadsAddr), imm(ICache));
      }
      emitCpuStep(96);
      mov32(IcacheTagKeyMem(lineIndex), imm(tagKey));
      mov64(reg(1), mem0(ramDataField));
      mov128(IcacheLineWordsMem(lineIndex, 0x00), mem(reg(1), sljit_sw(ramByteOff + 0x00)));
      mov128(IcacheLineWordsMem(lineIndex, 0x10), mem(reg(1), sljit_sw(ramByteOff + 0x10)));
    } else {
      emitEXECUTE(slow.instruction, true);
      emitDeferredCycles += slow.instructionCycles;
      flushDeferredCycles();
      test32(PipelineReg(state), imm(Pipeline::EndBlock), set_z);
      mov32(PipelineReg(state), PipelineReg(nstate));
      mov64(mem(IpuReg(pc)), PipelineReg(pc));
      if(slow.jumpEpilog) jumpEpilog(flag_nz);
    }
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), slow.resume);
  }

  memory::jitprotect(false);
  auto block = (Block*)allocator.acquire(sizeof(Block));
  block->code = endFunction();
  block->next = nullptr;
  block->linkedBlockTaken = nullptr;
  block->linkedBlockNotTaken = nullptr;
  block->stateKey = stateKey;
  block->startAddress = startAddress;
  block->endAddress = address;
  block->linkVaddrTaken      = branchLinksValid ? links.takenVaddr      : ~0ull;
  block->linkVaddrNotTaken   = branchLinksValid ? links.notTakenVaddr   : ~0ull;
  block->linkAddressTaken    = branchLinksValid ? links.takenAddress    : ~0u;
  block->linkAddressNotTaken = branchLinksValid ? links.notTakenAddress : ~0u;
  block->sectionDirty = sectionDirty.data() + startSection;

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
static constexpr s32 FpuCsrBaseOffset = offsetof(CPU, fpu) + offsetof(CPU::FPU, csr);
static constexpr s32 FpuCsrCauseOffset = FpuCsrBaseOffset + offsetof(CPU::FPU::ControlStatus, cause);
static constexpr s32 FpuR64S32Off  = offsetof(CPU::r64, s32);
static constexpr s32 FpuR64S32hOff = offsetof(CPU::r64, s32h);
static constexpr s32 RecompilerBaseOffset = offsetof(CPU, recompiler);
static constexpr s32 RecompilerFpuFastMxcsrOffset = RecompilerBaseOffset + offsetof(CPU::Recompiler, emitFpuFastMxcsr);
static constexpr s32 RecompilerFpuSaveMxcsrOffset = RecompilerBaseOffset + offsetof(CPU::Recompiler, emitFpuSaveMxcsr);
#define FpuCsrCompare mem(sreg(0), FpuCsrBaseOffset + offsetof(CPU::FPU::ControlStatus, compare))

static_assert(sizeof(n1) == 1);
static_assert(offsetof(CPU::FPU::ControlStatus::Cause, inexact) == 0);
static_assert(offsetof(CPU::FPU::ControlStatus::Cause, underflow) == 1);
static_assert(offsetof(CPU::FPU::ControlStatus::Cause, overflow) == 2);
static_assert(offsetof(CPU::FPU::ControlStatus::Cause, divisionByZero) == 3);
static_assert(offsetof(CPU::FPU::ControlStatus::Cause, invalidOperation) == 4);
static_assert(offsetof(CPU::FPU::ControlStatus::Cause, unimplementedOperation) == 5);
static_assert(sizeof(CPU::FPU::ControlStatus::Cause) == 6);
static_assert((RecompilerFpuFastMxcsrOffset & 3) == 0);
static_assert((RecompilerFpuSaveMxcsrOffset & 3) == 0);

#define XRd       IpuReg(r[0]) + XRdn * sizeof(r64)
#define XRt       IpuReg(r[0]) + XRtn * sizeof(r64)

#define i16 s16(instruction)
#define n16 u16(instruction)
#define n26 u32(instruction & 0x03ff'ffff)

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

auto CPU::Recompiler::setupCallf() -> void {
  if(emitCallfSetupDone) return;
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

auto CPU::Recompiler::jitMemoryOpcode(u32 instruction, u32 size, bool sign, bool require64, bool store,
  void (CPU::*loadInterpreter)(r64&, cr64&, s16),
  void (CPU::*storeInterpreter)(cr64&, cr64&, s16), bool emitSlowPath) -> void {
  if(emitSlowPath || emitSingleInstruction || (require64 && reservedInstruction64())) {
    setupCallf();
    if(store) {
      callf(storeInterpreter, mem(Rt), mem(Rs), imm(i16));
    } else {
      callf(loadInterpreter, mem(Rt), mem(Rs), imm(i16));
      emitZeroClear(Rtn);
    }
    return;
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
  if(size > Byte && !alignmentKnown) {
    test32(reg(0), imm(size - 1), set_z);
    addressUnaligned = jump(flag_nz);
  }

  // Convert the cached virtual address to an RDRAM physical address and locate its dcache line.
  and32(reg(0), reg(0), imm(0x007f'ffff));
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
  if(store) {
    if(system.homebrewMode) {
      and32(reg(3), reg(0), imm(0x0f));
      mov32(reg(1), imm((1 << size) - 1));
      shl32(reg(1), reg(1), reg(3));
    }

    if(size == Byte) {
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
      mov32_u16(reg(0), mem(reg(2), DcacheLineDirtyOff));
      or32(reg(1), reg(1), reg(0));
      mov32_u16(mem(reg(2), DcacheLineDirtyOff), reg(1));
      mov64(mem(reg(2), DcacheLineDirtyPcOff), imm(emitVaddr));
    } else {
      mov32_u16(mem(reg(2), DcacheLineDirtyOff), imm(1));
    }
  } else if(Rtn != 0) {
    if(size == Byte) {
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
      mov64_u32(reg(0), mem(reg(3), DcacheLineWordsOff + 0));
      mov64_u32(reg(1), mem(reg(3), DcacheLineWordsOff + 4));
      shl64(reg(0), reg(0), imm(32));
      or64(reg(3), reg(0), reg(1));
    }
    mov64(mem(Rt), reg(3));
  }

  // All failed fast-path guards share one generated slow path and return here afterwards.
  deferSlowPath({addressMismatch, addressOutOfRange, addressUnaligned, cacheMiss}, instruction);
}


auto CPU::Recompiler::emitEXECUTE(u32 instruction, bool emitSlowPath) -> bool {
  emitSlowPathSection = emitSlowPath;
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
    if(emitSlowPath) {
      setupCallf();
      callf(&CPU::ADDI, mem(Rt), mem(Rs), imm(i16));
      return 0;
    }
    add32(reg(0), mem(Rs32), imm(i16), set_o);
    auto overflow = jump(flag_o);
    if(Rtn != 0) {
      mov64_s32(reg(0), reg(0));
      mov64(mem(Rt), reg(0));
    }
    deferSlowPath(overflow, instruction);
    return 0;
  }

  //ADDIU Rt,Rs,i16
  case 0x09: {
    if(Rtn == 0) return 0;
    add32(reg(0), mem(Rs32), imm(i16));
    mov64_s32(reg(0), reg(0));
    mov64(mem(Rt), reg(0));
    if(Rtn == 29 && Rsn == 29) updateStackPointerStateKey(i16);
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
    setupCallf();
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
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPath || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADDI, mem(Rt), mem(Rs), imm(i16));
      return 0;
    }
    add64(reg(0), mem(Rs), imm(i16), set_o);
    auto overflow = jump(flag_o);
    if(Rtn != 0) mov64(mem(Rt), reg(0));
    deferSlowPath(overflow, instruction);
    return 0;
  }

  //DADDIU Rt,Rs,i16
  case 0x19: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPath || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADDIU, mem(Rt), mem(Rs), imm(i16));
      return 0;
    }
    add64(reg(0), mem(Rs), imm(i16));
    if(Rtn != 0) mov64(mem(Rt), reg(0));
    if(Rtn == 29 && Rsn == 29) updateStackPointerStateKey(i16);
    return 0;
  }

  //LDL Rt,Rs,i16
  case 0x1a: {
    setupCallf();
    callf(&CPU::LDL, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LDR Rt,Rs,i16
  case 0x1b: {
    setupCallf();
    callf(&CPU::LDR, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //INVALID
  case range4(0x1c, 0x1f): {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    jitMemoryOpcode(instruction, Byte, true, false, false, &CPU::LB, nullptr, emitSlowPath);
    return 0;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    jitMemoryOpcode(instruction, Half, true, false, false, &CPU::LH, nullptr, emitSlowPath);
    return 0;
  }

  //LWL Rt,Rs,i16
  case 0x22: {
    setupCallf();
    callf(&CPU::LWL, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }
  //LW Rt,Rs,i16
  case 0x23: {
    jitMemoryOpcode(instruction, Word, true, false, false, &CPU::LW, nullptr, emitSlowPath);
    return 0;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    jitMemoryOpcode(instruction, Byte, false, false, false, &CPU::LBU, nullptr, emitSlowPath);
    return 0;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    jitMemoryOpcode(instruction, Half, false, false, false, &CPU::LHU, nullptr, emitSlowPath);
    return 0;
  }

  //LWR Rt,Rs,i16
  case 0x26: {
    setupCallf();
    callf(&CPU::LWR, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LWU Rt,Rs,i16
  case 0x27: {
    jitMemoryOpcode(instruction, Word, false, false, false, &CPU::LWU, nullptr, emitSlowPath);
    return 0;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    jitMemoryOpcode(instruction, Byte, false, false, true, nullptr, &CPU::SB, emitSlowPath);
    return 0;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    jitMemoryOpcode(instruction, Half, false, false, true, nullptr, &CPU::SH, emitSlowPath);
    return 0;
  }

  //SWL Rt,Rs,i16
  case 0x2a: {
    setupCallf();
    callf(&CPU::SWL, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    jitMemoryOpcode(instruction, Word, false, false, true, nullptr, &CPU::SW, emitSlowPath);
    return 0;
  }

  //SDL Rt,Rs,i16
  case 0x2c: {
    setupCallf();
    callf(&CPU::SDL, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SDR Rt,Rs,i16
  case 0x2d: {
    setupCallf();
    callf(&CPU::SDR, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SWR Rt,Rs,i16
  case 0x2e: {
    setupCallf();
    callf(&CPU::SWR, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //CACHE op(offset),base
  case 0x2f: {
    setupCallf();
    callf(&CPU::CACHE, imm(instruction >> 16 & 31), mem(Rs), imm(i16));
    return 0;
  }

  //LL Rt,Rs,i16
  case 0x30: {
    setupCallf();
    callf(&CPU::LL, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LWC1 Ft,Rs,i16
  case 0x31: {
    setupCallf();
    callf(&CPU::LWC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //LWC2
  case 0x32: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //LWC3
  case 0x33: {
    setupCallf();
    callf(&CPU::COP3);
    return 1;
  }

  //LLD Rt,Rs,i16
  case 0x34: {
    setupCallf();
    callf(&CPU::LLD, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //LDC1 Ft,Rs,i16
  case 0x35: {
    setupCallf();
    callf(&CPU::LDC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //LDC2
  case 0x36: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //LD Rt,Rs,i16
  case 0x37: {
    jitMemoryOpcode(instruction, Dual, false, true, false, &CPU::LD, nullptr, emitSlowPath);
    return 0;
  }

  //SC Rt,Rs,i16
  case 0x38: {
    setupCallf();
    callf(&CPU::SC, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //SWC1 Ft,Rs,i16
  case 0x39: {
    setupCallf();
    callf(&CPU::SWC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //SWC2
  case 0x3a: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //SWC3
  case 0x3b: {
    setupCallf();
    callf(&CPU::COP3);
    return 1;
  }

  //SCD Rt,Rs,i16
  case 0x3c: {
    setupCallf();
    callf(&CPU::SCD, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return 0;
  }

  //SDC1 Ft,Rs,i16
  case 0x3d: {
    setupCallf();
    callf(&CPU::SDC1, imm(Ftn), mem(Rs), imm(i16));
    return 0;
  }

  //SDC2
  case 0x3e: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //SD Rt,Rs,i16
  case 0x3f: {
    jitMemoryOpcode(instruction, Dual, false, true, true, nullptr, &CPU::SD, emitSlowPath);
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
    setupCallf();
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
    setupCallf();
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
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //SYSCALL
  case 0x0c: {
    setupCallf();
    callf(&CPU::SYSCALL);
    return 1;
  }

  //BREAK
  case 0x0d: {
    setupCallf();
    callf(&CPU::BREAK);
    return 1;
  }

  //INVALID
  case 0x0e: {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //SYNC
  case 0x0f: {
    setupCallf();
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
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSLLV, mem(Rd), mem(Rt), mem(Rs));
      return 0;
    }
    if(Rdn == 0) return 0;
    and64(reg(1), mem(Rs), imm(63));
    shl64(reg(0), mem(Rt), reg(1));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //INVALID
  case 0x15: {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //DSRLV Rd,Rt,Rs
  case 0x16: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRLV, mem(Rd), mem(Rt), mem(Rs));
      return 0;
    }
    if(Rdn == 0) return 0;
    and64(reg(1), mem(Rs), imm(63));
    lshr64(reg(0), mem(Rt), reg(1));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //DSRAV Rd,Rt,Rs
  case 0x17: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRAV, mem(Rd), mem(Rt), mem(Rs));
      return 0;
    }
    if(Rdn == 0) return 0;
    and64(reg(1), mem(Rs), imm(63));
    ashr64(reg(0), mem(Rt), reg(1));
    mov64(mem(Rd), reg(0));
    return 0;
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
    return 0;
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
    return 0;
  }

  //DIV Rs,Rt
  case 0x1a: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DIV, mem(Rs), mem(Rt));
      return 0;
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
    return 0;
  }

  //DIVU Rs,Rt
  case 0x1b: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DIVU, mem(Rs), mem(Rt));
      return 0;
    }
    cmp32(mem(Rt32), imm(0), set_z);
    auto divByZero = jump(flag_z);
    divmod32_uw(reg(0), reg(1), mem(Rs32), mem(Rt32));
    mov64(mem(Lo), reg(0));
    mov64(mem(Hi), reg(1));
    deferSlowPath(divByZero, instruction);
    emitDeferredCycles += (37 - 1) * 2;
    return 0;
  }

  //DMULT Rs,Rt
  case 0x1c: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DMULT, mem(Rs), mem(Rt));
      return 0;
    }
    lmul64_sw(mem(Lo), mem(Hi), mem(Rs), mem(Rt));
    emitDeferredCycles += (8 - 1) * 2;
    return 0;
  }

  //DMULTU Rs,Rt
  case 0x1d: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DMULTU, mem(Rs), mem(Rt));
      return 0;
    }
    lmul64_uw(mem(Lo), mem(Hi), mem(Rs), mem(Rt));
    emitDeferredCycles += (8 - 1) * 2;
    return 0;
  }

  //DDIV Rs,Rt
  case 0x1e: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DDIV, mem(Rs), mem(Rt));
      return 0;
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
    return 0;
  }

  //DDIVU Rs,Rt
  case 0x1f: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DDIVU, mem(Rs), mem(Rt));
      return 0;
    }
    cmp64(mem(Rt), imm(0), set_z);
    auto divByZero = jump(flag_z);
    divmod64_uw(mem(Lo), mem(Hi), mem(Rs), mem(Rt));
    deferSlowPath(divByZero, instruction);
    emitDeferredCycles += (69 - 1) * 2;
    return 0;
  }

  //ADD Rd,Rs,Rt
  case 0x20: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::ADD, mem(Rd), mem(Rs), mem(Rt));
      return 0;
    }
    add32(reg(0), mem(Rs32), mem(Rt32), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) {
      mov64_s32(reg(0), reg(0));
      mov64(mem(Rd), reg(0));
    }
    deferSlowPath(overflow, instruction);
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
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::SUB, mem(Rd), mem(Rs), mem(Rt));
      return 0;
    }
    sub32(reg(0), mem(Rs32), mem(Rt32), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) {
      mov64_s32(reg(0), reg(0));
      mov64(mem(Rd), reg(0));
    }
    deferSlowPath(overflow, instruction);
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
    setupCallf();
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
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADD, mem(Rd), mem(Rs), mem(Rt));
      return 0;
    }
    add64(reg(0), mem(Rs), mem(Rt), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) mov64(mem(Rd), reg(0));
    deferSlowPath(overflow, instruction);
    return 0;
  }

  //DADDU Rd,Rs,Rt
  case 0x2d: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DADDU, mem(Rd), mem(Rs), mem(Rt));
      return 0;
    }
    if(Rdn == 0) return 0;
    add64(reg(0), mem(Rs), mem(Rt));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //DSUB Rd,Rs,Rt
  case 0x2e: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSUB, mem(Rd), mem(Rs), mem(Rt));
      return 0;
    }
    sub64(reg(0), mem(Rs), mem(Rt), set_o);
    auto overflow = jump(flag_o);
    if(Rdn != 0) mov64(mem(Rd), reg(0));
    deferSlowPath(overflow, instruction);
    return 0;
  }

  //DSUBU Rd,Rs,Rt
  case 0x2f: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSUBU, mem(Rd), mem(Rs), mem(Rt));
      return 0;
    }
    if(Rdn == 0) return 0;
    sub64(reg(0), mem(Rs), mem(Rt));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //TGE Rs,Rt
  case 0x30: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGE, mem(Rs), mem(Rt));
      return 0;
    }
    cmp64(mem(Rs), mem(Rt), set_slt);
    auto trap = jump(flag_sge);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TGEU Rs,Rt
  case 0x31: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGEU, mem(Rs), mem(Rt));
      return 0;
    }
    cmp64(mem(Rs), mem(Rt), set_ult);
    auto trap = jump(flag_uge);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TLT Rs,Rt
  case 0x32: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLT, mem(Rs), mem(Rt));
      return 0;
    }
    cmp64(mem(Rs), mem(Rt), set_slt);
    auto trap = jump(flag_slt);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TLTU Rs,Rt
  case 0x33: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLTU, mem(Rs), mem(Rt));
      return 0;
    }
    cmp64(mem(Rs), mem(Rt), set_ult);
    auto trap = jump(flag_ult);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TEQ Rs,Rt
  case 0x34: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TEQ, mem(Rs), mem(Rt));
      return 0;
    }
    cmp64(mem(Rs), mem(Rt), set_z);
    auto trap = jump(flag_z);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //INVALID
  case 0x35: {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //TNE Rs,Rt
  case 0x36: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TNE, mem(Rs), mem(Rt));
      return 0;
    }
    cmp64(mem(Rs), mem(Rt), set_z);
    auto trap = jump(flag_nz);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //INVALID
  case 0x37: {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }
  //DSLL Rd,Rt,Sa
  case 0x38: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSLL, mem(Rd), mem(Rt), imm(Sa));
      return 0;
    }
    if(Rdn == 0) return 0;
    shl64(reg(0), mem(Rt), imm(Sa));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //INVALID
  case 0x39: {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //DSRL Rd,Rt,Sa
  case 0x3a: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRL, mem(Rd), mem(Rt), imm(Sa));
      return 0;
    }
    if(Rdn == 0) return 0;
    lshr64(reg(0), mem(Rt), imm(Sa));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //DSRA Rd,Rt,Sa
  case 0x3b: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRA, mem(Rd), mem(Rt), imm(Sa));
      return 0;
    }
    if(Rdn == 0) return 0;
    ashr64(reg(0), mem(Rt), imm(Sa));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //DSLL32 Rd,Rt,Sa
  case 0x3c: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSLL, mem(Rd), mem(Rt), imm(Sa+32));
      return 0;
    }
    if(Rdn == 0) return 0;
    shl64(reg(0), mem(Rt), imm(Sa + 32));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //INVALID
  case 0x3d: {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //DSRL32 Rd,Rt,Sa
  case 0x3e: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRL, mem(Rd), mem(Rt), imm(Sa+32));
      return 0;
    }
    if(Rdn == 0) return 0;
    lshr64(reg(0), mem(Rt), imm(Sa + 32));
    mov64(mem(Rd), reg(0));
    return 0;
  }

  //DSRA32 Rd,Rt,Sa
  case 0x3f: {
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
      setupCallf();
      callf(&CPU::DSRA, mem(Rd), mem(Rt), imm(Sa+32));
      return 0;
    }
    if(Rdn == 0) return 0;
    ashr64(reg(0), mem(Rt), imm(Sa + 32));
    mov64(mem(Rd), reg(0));
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
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //TGEI Rs,i16
  case 0x08: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGEI, mem(Rs), imm(i16));
      return 0;
    }
    cmp64(mem(Rs), imm(i16), set_slt);
    auto trap = jump(flag_sge);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TGEIU Rs,i16
  case 0x09: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TGEIU, mem(Rs), imm(i16));
      return 0;
    }
    cmp64(mem(Rs), imm(i16), set_ult);
    auto trap = jump(flag_uge);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TLTI Rs,i16
  case 0x0a: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLTI, mem(Rs), imm(i16));
      return 0;
    }
    cmp64(mem(Rs), imm(i16), set_slt);
    auto trap = jump(flag_slt);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TLTIU Rs,i16
  case 0x0b: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TLTIU, mem(Rs), imm(i16));
      return 0;
    }
    cmp64(mem(Rs), imm(i16), set_ult);
    auto trap = jump(flag_ult);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //TEQI Rs,i16
  case 0x0c: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TEQI, mem(Rs), imm(i16));
      return 0;
    }
    cmp64(mem(Rs), imm(i16), set_z);
    auto trap = jump(flag_z);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //INVALID
  case 0x0d: {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //TNEI Rs,i16
  case 0x0e: {
    if(emitSlowPathSection) {
      setupCallf();
      callf(&CPU::TNEI, mem(Rs), imm(i16));
      return 0;
    }
    cmp64(mem(Rs), imm(i16), set_z);
    auto trap = jump(flag_nz);
    deferSlowPath(trap, instruction);
    return 0;
  }

  //INVALID
  case 0x0f: {
    setupCallf();
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
    setupCallf();
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
    setupCallf();
    callf(&CPU::MFC0, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //DMFC0 Rt,Rd
  case 0x01: {
    setupCallf();
    callf(&CPU::DMFC0, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //INVALID
  case range2(0x02, 0x03): {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    setupCallf();
    callf(&CPU::MTC0, mem(Rt), imm(Rdn));
    return 0;
  }

  //DMTC0 Rt,Rd
  case 0x05: {
    setupCallf();
    callf(&CPU::DMTC0, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case range10(0x06, 0x0f): {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  }

  switch(instruction & 0x3f) {

  //TLBR
  case 0x01: {
    setupCallf();
    callf(&CPU::TLBR);
    return 0;
  }

  //TLBWI
  case 0x02: {
    setupCallf();
    callf(&CPU::TLBWI);
    return 0;
  }

  //TLBWR
  case 0x06: {
    setupCallf();
    callf(&CPU::TLBWR);
    return 0;
  }

  //TLBP
  case 0x08: {
    setupCallf();
    callf(&CPU::TLBP);
    return 0;
  }

  //ERET
  case 0x18: {
    setupCallf();
    callf(&CPU::ERET);
    return 1;
  }

  //XDETECT
  case 0x20: {
    setupCallf();
    callf(&CPU::XDETECT, mem(XRd), imm(XCODE));
    return 0;
  }

  //XLOG
  case 0x25: {
    setupCallf();
    callf(&CPU::XLOG, mem(XRd), mem(XRt), imm(XCODE));
    return 0;
  }

  //XHEXDUMP
  case 0x27: {
    setupCallf();
    callf(&CPU::XHEXDUMP, mem(XRd), mem(XRt));
    return 0;
  }

  //XPROF
  case 0x28: {
    setupCallf();
    callf(&CPU::XPROF, mem(XRd), imm(XCODE));
    return 0;
  }

  //XPROFREAD
  case 0x29: {
    setupCallf();
    callf(&CPU::XPROFREAD, mem(XRd), mem(XRt));
    return 0;
  }

  //XEXCEPTION
  case 0x2a: {
    setupCallf();
    callf(&CPU::XEXCEPTION, mem(XRt));
    return 0;
  }

  //XIOCTL
  case 0x2c: {
    setupCallf();
    callf(&CPU::XIOCTL, imm(XCODE));
    return 0;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitFPU(u32 instruction) -> bool {
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
  auto arm64RegIndex = [&](reg r) -> s32 {
    return sljit_get_register_index(SLJIT_GP_REGISTER, r.fst);
  };
  auto arm64RegBits0 = [&](reg r) -> u32 {
    auto index = arm64RegIndex(r);
    assert(index >= 0);
    return u32(index);
  };
  auto arm64RegBits5 = [&](reg r) -> u32 {
    auto index = arm64RegIndex(r);
    assert(index >= 0);
    return u32(index) << 5;
  };
  auto arm64Emit = [&](u32 opcode) -> void {
    sljit_emit_op_custom(compiler, &opcode, sizeof(opcode));
  };
  auto arm64ReadFpcr = [&](reg rt) -> void {
    arm64Emit(0xd53b4400u | arm64RegBits0(rt));
  };
  auto arm64WriteFpcr = [&](reg rt) -> void {
    arm64Emit(0xd51b4400u | arm64RegBits0(rt));
  };
  auto arm64ReadFpsr = [&](reg rt) -> void {
    arm64Emit(0xd53b4420u | arm64RegBits0(rt));
  };
  auto arm64WriteFpsr = [&](reg rt) -> void {
    arm64Emit(0xd51b4420u | arm64RegBits0(rt));
  };
  auto arm64MoveWToS0 = [&](reg rt) -> void {
    arm64Emit(0x1e270000u | arm64RegBits5(rt));
  };
  auto arm64MoveS0ToW = [&](reg rt) -> void {
    arm64Emit(0x1e260000u | arm64RegBits0(rt));
  };
  auto arm64FsqrtS0 = [&]() -> void {
    arm64Emit(0x1e21c000u);
  };
  auto arm64FabsS0 = [&]() -> void {
    arm64Emit(0x1e20c000u);
  };
  auto arm64FnegS0 = [&]() -> void {
    arm64Emit(0x1e214000u);
  };
  auto arm64RoundModeBits = [&]() -> u32 {
    switch(emitStateKey.fpuRoundMode()) {
    case 0: return 0x0000'0000u;
    case 1: return 0x00c0'0000u;
    case 2: return 0x0040'0000u;
    case 3: return 0x0080'0000u;
    }
    unreachable;
  };
  auto arm64FpuFastFpcr = [&]() -> u32 {
    return 0x0180'0000u | arm64RoundModeBits();
  };
#elif defined(ARCHITECTURE_AMD64)
  auto amd64RegIndex = [&](reg r) -> s32 {
    return sljit_get_register_index(SLJIT_GP_REGISTER, r.fst);
  };
  auto amd64Emit = [&](std::initializer_list<u8> bytes) -> void {
    u8 opcode[16];
    assert(bytes.size() <= sizeof(opcode));
    u32 n = 0;
    for(auto byte : bytes) opcode[n++] = byte;
    sljit_emit_op_custom(compiler, opcode, n);
  };
  auto amd64EmitModRMDisp32 = [&](u32 ext, s32 offset) -> void {
    s32 base = sljit_get_register_index(SLJIT_GP_REGISTER, sreg(0).fst);
    assert(base >= 0);
    u8 opcode[10];
    u32 n = 0;
    u8 rex = 0x40u | (u8(base) >> 3 & 1);
    if(rex != 0x40u) opcode[n++] = rex;
    opcode[n++] = 0x0f;
    opcode[n++] = 0xae;
    opcode[n++] = 0x80u | (u8(ext & 7) << 3) | (u8(base) & 7);
    if((u8(base) & 7) == 4) opcode[n++] = 0x24;
    u32 disp = u32(offset);
    opcode[n++] = disp >> 0;
    opcode[n++] = disp >> 8;
    opcode[n++] = disp >> 16;
    opcode[n++] = disp >> 24;
    sljit_emit_op_custom(compiler, opcode, n);
  };
  auto amd64MoveWToS = [&](u32 sd, reg rt) -> void {
    s32 src = amd64RegIndex(rt);
    assert(src >= 0);
    u8 opcode[8];
    u32 n = 0;
    opcode[n++] = 0x66;
    u8 rex = 0x40u | (u8(sd) >> 3 & 1) << 2 | (u8(src) >> 3 & 1);
    if(rex != 0x40u) opcode[n++] = rex;
    opcode[n++] = 0x0f;
    opcode[n++] = 0x6e;
    opcode[n++] = 0xc0u | (u8(sd) & 7) << 3 | (u8(src) & 7);
    sljit_emit_op_custom(compiler, opcode, n);
  };
  auto amd64MoveSToW = [&](reg rt, u32 ss) -> void {
    s32 dst = amd64RegIndex(rt);
    assert(dst >= 0);
    u8 opcode[8];
    u32 n = 0;
    opcode[n++] = 0x66;
    u8 rex = 0x40u | (u8(ss) >> 3 & 1) << 2 | (u8(dst) >> 3 & 1);
    if(rex != 0x40u) opcode[n++] = rex;
    opcode[n++] = 0x0f;
    opcode[n++] = 0x7e;
    opcode[n++] = 0xc0u | (u8(ss) & 7) << 3 | (u8(dst) & 7);
    sljit_emit_op_custom(compiler, opcode, n);
  };
  auto amd64Stmxcsr = [&](s32 offset) -> void {
    amd64EmitModRMDisp32(3, offset);
  };
  auto amd64Ldmxcsr = [&](s32 offset) -> void {
    amd64EmitModRMDisp32(2, offset);
  };
  auto amd64MoveWToS0 = [&](reg rt) -> void {
    amd64MoveWToS(0, rt);
  };
  auto amd64MoveWToS1 = [&](reg rt) -> void {
    amd64MoveWToS(1, rt);
  };
  auto amd64MoveS0ToW = [&](reg rt) -> void {
    amd64MoveSToW(rt, 0);
  };
  auto amd64FsqrtS0 = [&]() -> void {
    amd64Emit({0xf3, 0x0f, 0x51, 0xc0});
  };
  auto amd64FabsS0 = [&]() -> void {
    mov32(reg(2), imm(0x7fff'ffff));
    amd64MoveWToS1(reg(2));
    amd64Emit({0x0f, 0x54, 0xc1});
  };
  auto amd64FnegS0 = [&]() -> void {
    mov32(reg(2), imm(0x8000'0000));
    amd64MoveWToS1(reg(2));
    amd64Emit({0x0f, 0x57, 0xc1});
  };
  auto amd64RoundModeBits = [&]() -> u32 {
    switch(emitStateKey.fpuRoundMode()) {
    case 0: return 0x0000'0000u;
    case 1: return 0x0000'6000u;
    case 2: return 0x0000'4000u;
    case 3: return 0x0000'2000u;
    }
    unreachable;
  };
  auto amd64FpuFastMxcsr = [&]() -> u32 {
    return 0x0000'9f80u | amd64RoundModeBits();
  };
#endif
  auto fpuSingleSourceWordOffset = [&](u32 fsn) -> s32 {
    if(emitStateKey.floatingPointMode()) return (fsn - 16) * 8 + FpuR64S32Off;
    return ((fsn & ~1) - 16) * 8 + FpuR64S32Off;
  };
  auto fpuSingleDestWordOffset = [&](u32 fdn) -> s32 {
    return (fdn - 16) * 8 + FpuR64S32Off;
  };
  auto fpuSingleDestWordhOffset = [&](u32 fdn) -> s32 {
    return (fdn - 16) * 8 + FpuR64S32hOff;
  };
  enum FpuInputCheck : u32 {
    FpuCheckNone = 0,
    FpuCheckQnan = 1 << 0,
    FpuCheckSnan = 1 << 1,
    FpuCheckSubnormal = 1 << 2,
  };
  auto emitFpuOpcode = [&](
    void (CPU::*slowPath)(u8, u8), u32 fdn, u32 fsn, u32 cycles,
    u32 inputChecks, auto&& emitHostOpcode
  ) -> void {
    if(emitSlowPathSection || !emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(slowPath, imm(fdn), imm(fsn));
      return;
    }

    s32 fsWordOff = fpuSingleSourceWordOffset(fsn);
    s32 fdWordOff = fpuSingleDestWordOffset(fdn);
    s32 fdWordhOff = fpuSingleDestWordhOffset(fdn);

    movzeron(FpuCsrCauseOffset, sizeof(CPU::FPU::ControlStatus::Cause));
    mov32(reg(0), mem(sreg(2), fsWordOff));

    bool checkQnan = inputChecks & FpuCheckQnan;
    bool checkSnan = inputChecks & FpuCheckSnan;
    bool checkSubnormal = inputChecks & FpuCheckSubnormal;
    sljit_jump* qnan = nullptr;
    sljit_jump* snan = nullptr;
    sljit_jump* subnormal = nullptr;
    if(checkSubnormal) {
      and32(reg(1), reg(0), imm(0x7f80'0000));
      cmp32(reg(1), imm(0), set_z);
      auto expNonZero = jump(flag_nz);
      and32(reg(1), reg(0), imm(0x007f'ffff));
      cmp32(reg(1), imm(0), set_z);
      subnormal = jump(flag_nz);
      setLabel(expNonZero);
    }
    if(checkQnan && !checkSnan) {
      and32(reg(1), reg(0), imm(0x7fc0'0000));
      cmp32(reg(1), imm(0x7fc0'0000), set_z);
      qnan = jump(flag_z);
    } else if(checkQnan || checkSnan) {
      and32(reg(1), reg(0), imm(0x7f80'0000));
      cmp32(reg(1), imm(0x7f80'0000), set_z);
      auto expNotAllOnes = jump(flag_nz);
      and32(reg(1), reg(0), imm(0x007f'ffff));
      cmp32(reg(1), imm(0), set_z);
      auto inf = jump(flag_z);
      and32(reg(1), reg(0), imm(0x0040'0000));
      cmp32(reg(1), imm(0), set_z);
      if(checkQnan) qnan = jump(flag_nz);
      if(checkSnan) snan = jump(flag_z);
      setLabel(inf);
      setLabel(expNotAllOnes);
    }

    sljit_jump* weird = nullptr;
#if defined(ARCHITECTURE_ARM64)
    constexpr u32 fpuStatusMask = 0xbf; // stick bits (0x80 is for denormals)
    arm64ReadFpcr(reg(2));
    mov64(reg(3), imm(arm64FpuFastFpcr()));
    arm64WriteFpcr(reg(3));
    mov64(reg(3), imm(0));
    arm64WriteFpsr(reg(3));

    arm64MoveWToS0(reg(0));
    emitHostOpcode();
    arm64MoveS0ToW(reg(1));

    arm64ReadFpsr(reg(3));
    arm64WriteFpcr(reg(2));
    test64(reg(3), imm(fpuStatusMask), set_z);
    weird = jump(flag_nz);
#elif defined(ARCHITECTURE_AMD64)
    constexpr u32 fpuStatusMask = 0x3f;
    amd64Stmxcsr(RecompilerFpuSaveMxcsrOffset);
    mov32(mem(sreg(0), RecompilerFpuFastMxcsrOffset), imm(amd64FpuFastMxcsr()));
    amd64Ldmxcsr(RecompilerFpuFastMxcsrOffset);

    amd64MoveWToS0(reg(0));
    emitHostOpcode();
    amd64MoveS0ToW(reg(1));

    amd64Stmxcsr(RecompilerFpuFastMxcsrOffset);
    amd64Ldmxcsr(RecompilerFpuSaveMxcsrOffset);
    mov32(reg(3), mem(sreg(0), RecompilerFpuFastMxcsrOffset));
    test32(reg(3), imm(fpuStatusMask), set_z);
    weird = jump(flag_nz);
#endif

    mov32(mem(sreg(2), fdWordOff), reg(1));
    mov32(mem(sreg(2), fdWordhOff), imm(0));

    deferSlowPath({qnan, snan, subnormal, weird}, instruction);
    emitDeferredCycles += cycles;
  };
#endif

  switch(instruction >> 21 & 0x1f) {

  //MFC1 Rt,Fs
  case 0x00: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::MFC1, mem(Rt), imm(Fsn));
      return 0;
    }
    if(Rtn == 0) return 0;
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
    return 0;
  }

  //DMFC1 Rt,Fs
  case 0x01: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::DMFC1, mem(Rt), imm(Fsn));
      return 0;
    }
    if(Rtn == 0) return 0;
    s32 fsn = instruction >> 11 & 31;
    s32 fpu64Off;
    if(emitStateKey.floatingPointMode()) fpu64Off = (fsn - 16) * 8;
    else fpu64Off = ((fsn & ~1) - 16) * 8;
    mov64(reg(0), mem(sreg(2), fpu64Off));
    mov64(mem(Rt), reg(0));
    return 0;
  }

  //CFC1 Rt,Rd
  case 0x02: {
    setupCallf();
    callf(&CPU::CFC1, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //DCFC1 Rt,Rd
  case 0x03: {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //MTC1 Rt,Fs
  case 0x04: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::MTC1, mem(Rt), imm(Fsn));
      return 0;
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
    return 0;
  }

  //DMTC1 Rt,Fs
  case 0x05: {
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::DMTC1, mem(Rt), imm(Fsn));
      return 0;
    }
    s32 fsn = instruction >> 11 & 31;
    s32 fpu64Off;
    if(emitStateKey.floatingPointMode()) fpu64Off = (fsn - 16) * 8;
    else fpu64Off = ((fsn & ~1) - 16) * 8;
    mov64(reg(0), mem(Rt));
    mov64(mem(sreg(2), fpu64Off), reg(0));
    return 0;
  }

  //CTC1 Rt,Rd
  case 0x06: {
    setupCallf();
    callf(&CPU::CTC1, mem(Rt), imm(Rdn));
    return 0;
  }

  //DCTC1 Rt,Rd
  case 0x07: {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //BC1 offset
  case 0x08: {
    bool value = instruction >> 16 & 1;
    bool likely = instruction >> 17 & 1;
    if(!emitStateKey.coprocessor1Enabled()) {
      setupCallf();
      callf(&CPU::BC1, imm(value), imm(likely), imm(i16));
      return 1;
    }
    movzeron(FpuCsrCauseOffset, sizeof(CPU::FPU::ControlStatus::Cause));
    mov32(reg(0), FpuCsrCompare);
    and32(reg(0), reg(0), imm(1));
    cmp32(reg(0), imm(value), set_z);
    auto taken = jump(flag_z);
    if(likely) {
      add64(reg(0), PipelineReg(pc), imm(4));
      mov64(PipelineReg(pc), reg(0));
      add64(PipelineReg(nextpc), reg(0), imm(4));
      or32(PipelineReg(state), PipelineReg(state), imm(Pipeline::EndBlock));
    } else {
      mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    }
    auto done = jump();
    setLabel(taken);
    add64(reg(0), PipelineReg(pc), imm(s32(i16) * 4));
    mov64(PipelineReg(nextpc), reg(0));
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return 1;
  }

  //INVALID
  case range7(0x09, 0x0f): {
    setupCallf();
    callf(&CPU::INVALID);
    return 1;
  }

  }

  if((instruction >> 21 & 31) == 16)
  switch(instruction & 0x3f) {

  //FADD.S Fd,Fs,Ft
  case 0x00: {
    setupCallf();
    callf(&CPU::FADD_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSUB.S Fd,Fs,Ft
  case 0x01: {
    setupCallf();
    callf(&CPU::FSUB_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FMUL.S Fd,Fs,Ft
  case 0x02: {
    setupCallf();
    callf(&CPU::FMUL_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FDIV.S Fd,Fs,Ft
  case 0x03: {
    setupCallf();
    callf(&CPU::FDIV_S, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSQRT.S Fd,Fs
  case 0x04: {
#if defined(ARCHITECTURE_ARM64) || defined(ARCHITECTURE_AMD64)
    emitFpuOpcode(
      &CPU::FSQRT_S, Fdn, Fsn, (29 - 1) * 2, FpuCheckQnan,
      [&]() -> void {
#if defined(ARCHITECTURE_ARM64)
        arm64FsqrtS0();
#elif defined(ARCHITECTURE_AMD64)
        amd64FsqrtS0();
#endif
      }
    );
    return 0;
#else
    setupCallf();
    callf(&CPU::FSQRT_S, imm(Fdn), imm(Fsn));
    return 0;
#endif
  }

  //FABS.S Fd,Fs
  case 0x05: {
#if defined(ARCHITECTURE_ARM64) || defined(ARCHITECTURE_AMD64)
    emitFpuOpcode(
      &CPU::FABS_S, Fdn, Fsn, 0, FpuCheckQnan | FpuCheckSnan | FpuCheckSubnormal,
      [&]() -> void {
#if defined(ARCHITECTURE_ARM64)
        arm64FabsS0();
#elif defined(ARCHITECTURE_AMD64)
        amd64FabsS0();
#endif
      }
    );
    return 0;
#else
    setupCallf();
    callf(&CPU::FABS_S, imm(Fdn), imm(Fsn));
    return 0;
#endif
  }

  //FMOV.S Fd,Fs
  case 0x06: {
    setupCallf();
    callf(&CPU::FMOV_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FNEG.S Fd,Fs
  case 0x07: {
#if defined(ARCHITECTURE_ARM64) || defined(ARCHITECTURE_AMD64)
    emitFpuOpcode(
      &CPU::FNEG_S, Fdn, Fsn, 0, FpuCheckQnan | FpuCheckSnan | FpuCheckSubnormal,
      [&]() -> void {
#if defined(ARCHITECTURE_ARM64)
        arm64FnegS0();
#elif defined(ARCHITECTURE_AMD64)
        amd64FnegS0();
#endif
      }
    );
    return 0;
#else
    setupCallf();
    callf(&CPU::FNEG_S, imm(Fdn), imm(Fsn));
    return 0;
#endif
  }

  //FROUND.L.S Fd,Fs
  case 0x08: {
    setupCallf();
    callf(&CPU::FROUND_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.L.S Fd,Fs
  case 0x09: {
    setupCallf();
    callf(&CPU::FTRUNC_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.L.S Fd,Fs
  case 0x0a: {
    setupCallf();
    callf(&CPU::FCEIL_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.L.S Fd,Fs
  case 0x0b: {
    setupCallf();
    callf(&CPU::FFLOOR_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FROUND.W.S Fd,Fs
  case 0x0c: {
    setupCallf();
    callf(&CPU::FROUND_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.W.S Fd,Fs
  case 0x0d: {
    setupCallf();
    callf(&CPU::FTRUNC_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.W.S Fd,Fs
  case 0x0e: {
    setupCallf();
    callf(&CPU::FCEIL_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.W.S Fd,Fs
  case 0x0f: {
    setupCallf();
    callf(&CPU::FFLOOR_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.S.S Fd,Fs
  case 0x20: {
    setupCallf();
    callf(&CPU::FCVT_S_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.S Fd,Fs
  case 0x21: {
    setupCallf();
    callf(&CPU::FCVT_D_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.W.S Fd,Fs
  case 0x24: {
    setupCallf();
    callf(&CPU::FCVT_W_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.L.S Fd,Fs
  case 0x25: {
    setupCallf();
    callf(&CPU::FCVT_L_S, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FC.F.S Fs,Ft
  case 0x30: {
    setupCallf();
    callf(&CPU::FC_F_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UN.S Fs,Ft
  case 0x31: {
    setupCallf();
    callf(&CPU::FC_UN_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.EQ.S Fs,Ft
  case 0x32: {
    setupCallf();
    callf(&CPU::FC_EQ_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UEQ.S Fs,Ft
  case 0x33: {
    setupCallf();
    callf(&CPU::FC_UEQ_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLT.S Fs,Ft
  case 0x34: {
    setupCallf();
    callf(&CPU::FC_OLT_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULT.S Fs,Ft
  case 0x35: {
    setupCallf();
    callf(&CPU::FC_ULT_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLE.S Fs,Ft
  case 0x36: {
    setupCallf();
    callf(&CPU::FC_OLE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULE.S Fs,Ft
  case 0x37: {
    setupCallf();
    callf(&CPU::FC_ULE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SF.S Fs,Ft
  case 0x38: {
    setupCallf();
    callf(&CPU::FC_SF_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGLE.S Fs,Ft
  case 0x39: {
    setupCallf();
    callf(&CPU::FC_NGLE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SEQ.S Fs,Ft
  case 0x3a: {
    setupCallf();
    callf(&CPU::FC_SEQ_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGL.S Fs,Ft
  case 0x3b: {
    setupCallf();
    callf(&CPU::FC_NGL_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LT.S Fs,Ft
  case 0x3c: {
    setupCallf();
    callf(&CPU::FC_LT_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGE.S Fs,Ft
  case 0x3d: {
    setupCallf();
    callf(&CPU::FC_NGE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LE.S Fs,Ft
  case 0x3e: {
    setupCallf();
    callf(&CPU::FC_LE_S, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGT.S Fs,Ft
  case 0x3f: {
    setupCallf();
    callf(&CPU::FC_NGT_S, imm(Fsn), imm(Ftn));
    return 0;
  }
  }

  if((instruction >> 21 & 31) == 17)
  switch(instruction & 0x3f) {

//FADD.D Fd,Fs,Ft
  case 0x00: {
    setupCallf();
    callf(&CPU::FADD_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSUB.D Fd,Fs,Ft
  case 0x01: {
    setupCallf();
    callf(&CPU::FSUB_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FMUL.D Fd,Fs,Ft
  case 0x02: {
    setupCallf();
    callf(&CPU::FMUL_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FDIV.D Fd,Fs,Ft
  case 0x03: {
    setupCallf();
    callf(&CPU::FDIV_D, imm(Fdn), imm(Fsn), imm(Ftn));
    return 0;
  }

  //FSQRT.D Fd,Fs
  case 0x04: {
    setupCallf();
    callf(&CPU::FSQRT_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FABS.D Fd,Fs
  case 0x05: {
    setupCallf();
    callf(&CPU::FABS_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FMOV.D Fd,Fs
  case 0x06: {
    setupCallf();
    callf(&CPU::FMOV_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FNEG.D Fd,Fs
  case 0x07: {
    setupCallf();
    callf(&CPU::FNEG_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FROUND.L.D Fd,Fs
  case 0x08: {
    setupCallf();
    callf(&CPU::FROUND_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.L.D Fd,Fs
  case 0x09: {
    setupCallf();
    callf(&CPU::FTRUNC_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.L.D Fd,Fs
  case 0x0a: {
    setupCallf();
    callf(&CPU::FCEIL_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.L.D Fd,Fs
  case 0x0b: {
    setupCallf();
    callf(&CPU::FFLOOR_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FROUND.W.D Fd,Fs
  case 0x0c: {
    setupCallf();
    callf(&CPU::FROUND_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FTRUNC.W.D Fd,Fs
  case 0x0d: {
    setupCallf();
    callf(&CPU::FTRUNC_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCEIL.W.D Fd,Fs
  case 0x0e: {
    setupCallf();
    callf(&CPU::FCEIL_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FFLOOR.W.D Fd,Fs
  case 0x0f: {
    setupCallf();
    callf(&CPU::FFLOOR_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.S.D Fd,Fs
  case 0x20: {
    setupCallf();
    callf(&CPU::FCVT_S_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.D Fd,Fs
  case 0x21: {
    setupCallf();
    callf(&CPU::FCVT_D_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.W.D Fd,Fs
  case 0x24: {
    setupCallf();
    callf(&CPU::FCVT_W_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.L.D Fd,Fs
  case 0x25: {
    setupCallf();
    callf(&CPU::FCVT_L_D, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FC.F.D Fs,Ft
  case 0x30: {
    setupCallf();
    callf(&CPU::FC_F_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UN.D Fs,Ft
  case 0x31: {
    setupCallf();
    callf(&CPU::FC_UN_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.EQ.D Fs,Ft
  case 0x32: {
    setupCallf();
    callf(&CPU::FC_EQ_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.UEQ.D Fs,Ft
  case 0x33: {
    setupCallf();
    callf(&CPU::FC_UEQ_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLT.D Fs,Ft
  case 0x34: {
    setupCallf();
    callf(&CPU::FC_OLT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULT.D Fs,Ft
  case 0x35: {
    setupCallf();
    callf(&CPU::FC_ULT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.OLE.D Fs,Ft
  case 0x36: {
    setupCallf();
    callf(&CPU::FC_OLE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.ULE.D Fs,Ft
  case 0x37: {
    setupCallf();
    callf(&CPU::FC_ULE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SF.D Fs,Ft
  case 0x38: {
    setupCallf();
    callf(&CPU::FC_SF_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGLE.D Fs,Ft
  case 0x39: {
    setupCallf();
    callf(&CPU::FC_NGLE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.SEQ.D Fs,Ft
  case 0x3a: {
    setupCallf();
    callf(&CPU::FC_SEQ_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGL.D Fs,Ft
  case 0x3b: {
    setupCallf();
    callf(&CPU::FC_NGL_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LT.D Fs,Ft
  case 0x3c: {
    setupCallf();
    callf(&CPU::FC_LT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGE.D Fs,Ft
  case 0x3d: {
    setupCallf();
    callf(&CPU::FC_NGE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.LE.D Fs,Ft
  case 0x3e: {
    setupCallf();
    callf(&CPU::FC_LE_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  //FC.NGT.D Fs,Ft
  case 0x3f: {
    setupCallf();
    callf(&CPU::FC_NGT_D, imm(Fsn), imm(Ftn));
    return 0;
  }

  }

  if((instruction >> 21 & 31) == 20)
  switch(instruction & 0x3f) {
  case range8(0x08, 0x0f): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  case range2(0x24, 0x25): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //FCVT.S.W Fd,Fs
  case 0x20: {
    setupCallf();
    callf(&CPU::FCVT_S_W, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.W Fd,Fs
  case 0x21: {
    setupCallf();
    callf(&CPU::FCVT_D_W, imm(Fdn), imm(Fsn));
    return 0;
  }

  }

  if((instruction >> 21 & 31) == 21)
  switch(instruction & 0x3f) {
  case range8(0x08, 0x0f): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }
  case range2(0x24, 0x25): {
    setupCallf();
    callf(&CPU::COP1UNIMPLEMENTED);
    return 1;
  }

  //FCVT.S.L
  case 0x20: {
    setupCallf();
    callf(&CPU::FCVT_S_L, imm(Fdn), imm(Fsn));
    return 0;
  }

  //FCVT.D.L
  case 0x21: {
    setupCallf();
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
    setupCallf();
    callf(&CPU::MFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //DMFC2 Rt,Rd
  case 0x01: {
    setupCallf();
    callf(&CPU::DMFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    setupCallf();
    callf(&CPU::CFC2, mem(Rt), imm(Rdn));
    emitZeroClear(Rtn);
    return 0;
  }

  //INVALID
  case 0x03: {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return 1;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    setupCallf();
    callf(&CPU::MTC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //DMTC2 Rt,Rd
  case 0x05: {
    setupCallf();
    callf(&CPU::DMTC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    setupCallf();
    callf(&CPU::CTC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case range9(0x07, 0x0f): {
    setupCallf();
    callf(&CPU::COP2INVALID);
    return 1;
  }

  }
  return 0;
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
