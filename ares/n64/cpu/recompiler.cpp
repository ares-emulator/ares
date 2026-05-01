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
   - on cache miss, emit() first runs a prepass, then builds host code.
3) Publish
   - the main block is inserted at startAddress.
   - internal branch targets are also published as alias entries that point to
     the same generated host function.

Block definition (current)
--------------------------
A compiled block is now a planned linear window of guest instructions plus
internal entry aliases:
- startAddress is the lookup address used to build the primary block entry;
- the window may include many conditional branches (including likely variants);
- the window closes on hard boundaries:
  section boundary, single-instruction mode, non-branch stateKey-changing ops,
  SCC Count/Compare writes, and delay-slot completion after unconditional jumps.

Emission pipeline inside emit()
------------------------------
emit() runs in phases:
1) Validation:
   - reject non-cacheable or incoherent starts.
2) Prepass planning:
   - decode OpInfo for each instruction in the linear window;
   - compute branch taken/fallthrough targets;
   - classify conditional branch edges as internal or external;
   - collect internal entry vaddrs and alias addresses.
3) Host emission:
   - emit an entry dispatcher that can jump directly to internal labels when
     ipu.pc matches an internal target;
   - emit each planned instruction body via emitEXECUTE()/emitSPECIAL()/...;
   - after each conditional branch delay slot, dispatch to internal targets or
     return to epilogue for external flow.
4) Finalization:
   - emit common epilogue and deferred slow paths;
   - publish block metadata.

Control-flow policy (current)
-----------------------------
- Conditional branches are not terminals by default.
- Internal conditional edges are resolved immediately with host-side jumps.
- External conditional edges return to dispatcher at block epilogue.
- Unconditional branch delay-slot completion is a hard planning boundary.
- Opportunistic runtime branch chaining is intentionally disabled.

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
  stateKey.setFpuInexactEnabled(self.fpu.csr.enable.inexact());
  stateKey.setFpuUnderflowEnabled(self.fpu.csr.enable.underflow());
  stateKey.setFpuOverflowEnabled(self.fpu.csr.enable.overflow());
  stateKey.setFpuDivisionByZeroEnabled(self.fpu.csr.enable.divisionByZero());
  stateKey.setFpuInvalidOperationEnabled(self.fpu.csr.enable.invalidOperation());
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
  auto vaddrPage = vaddr & ~0xfffull;
  for(auto block = section->blocks[index]; block; block = block->next) {
    if(block->stateKey == stateKey && block->vaddrPage == vaddrPage) {
      return block;
    }
  }

  auto block = emit(vaddr, address, stateKey, singleInstruction);
  if(block) {
    if(emitAllocatorFlushed) {
      section = this->section(address);
      if(!section) return nullptr;
    }
    block->next = section->blocks[index];
    section->blocks[index] = block;
    u32 firstLine = sectionLineIndex(block->startAddress);
    u32 lastLine = sectionLineIndex(block->endAddress - 1);
    for(u32 line = firstLine; line <= lastLine; line++) {
      section->lineBlocks[line] = 1;
    }
    block->sectionDirty = sectionDirty.data() + sectionIndex(block->startAddress);
    auto registerAlias = [&](u32 aliasAddress) -> Block* {
      if(aliasAddress == block->startAddress) return block;
      auto aliasIndex = blockIndex(aliasAddress);
      for(auto alias = section->blocks[aliasIndex]; alias; alias = alias->next) {
        if(alias->stateKey != block->stateKey) continue;
        if(alias->vaddrPage != block->vaddrPage) continue;
        if(alias->startAddress != aliasAddress) continue;
        return alias;
      }
      auto alias = (Block*)allocator.acquire(sizeof(Block));
      alias->code = block->code;
      alias->next = section->blocks[aliasIndex];
      alias->stateKey = block->stateKey;
      alias->vaddrPage = block->vaddrPage;
      alias->startAddress = aliasAddress;
      alias->endAddress = block->endAddress;
      alias->sectionDirty = block->sectionDirty;
      section->blocks[aliasIndex] = alias;
      section->lineBlocks[sectionLineIndex(aliasAddress)] = 1;
      return alias;
    };
    for(auto aliasAddress : emitAliasAddresses) {
      registerAlias(aliasAddress);
    }
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
#define CpuJitClockTargetMem mem(sreg(0), offsetof(CPU, jitClockTarget))

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

namespace {

struct EmitPlannedInstruction {
  u64 vaddr = 0;
  u32 address = 0;
  u32 instruction = 0;
  ares::Nintendo64::CPU::OpInfo info = {};
  bool countCompareWrite = false;
  bool isUnconditional = false;
  u64 branchTakenVaddr = ~0ull;
  u64 branchFallthroughVaddr = ~0ull;
  bool branchTakenInternal = false;
  bool branchFallthroughInternal = false;
};

struct EmitPlan {
  u32 startAddress = 0;
  u32 startSection = 0;
  u64 startVaddr = 0;
  u32 windowEndAddress = 0;
  u64 windowEndVaddr = 0;
  std::vector<EmitPlannedInstruction> instructions;
  std::vector<u64> internalEntryVaddrs;
};

// Decode branch-like instructions into taken/fallthrough virtual targets.
auto computeBranchTargets(bool coprocessor1Enabled, u64 branchVaddr, u32 instruction) -> std::pair<u64, u64> {
  // The plan stage keeps target decoding local and deterministic.
  auto opcode = instruction >> 26;
  auto rt = instruction >> 16 & 31;
  auto branchTargetVaddr = branchVaddr + 4 + (s64(s16(instruction)) << 2);
  auto fallthroughVaddr = branchVaddr + 8;
  if(opcode == 0x02 || opcode == 0x03) {
    // Absolute jump within the current 256 MiB region.
    auto t = (branchVaddr & 0xffff'ffff'f000'0000ull) | (u64(instruction & 0x03ff'ffff) << 2);
    return {t, ~0ull};
  }
  if(opcode == 0x04 || opcode == 0x05 || opcode == 0x06 || opcode == 0x07
  || opcode == 0x14 || opcode == 0x15 || opcode == 0x16 || opcode == 0x17) {
    return {branchTargetVaddr, fallthroughVaddr};
  }
  if(opcode == 0x01) {
    // REGIMM branch family.
    if(rt == 0x00 || rt == 0x01 || rt == 0x02 || rt == 0x03
    || rt == 0x10 || rt == 0x11 || rt == 0x12 || rt == 0x13) {
      return {branchTargetVaddr, fallthroughVaddr};
    }
    return {~0ull, ~0ull};
  }
  if(opcode == 0x11 && (instruction >> 21 & 31) == 0x08) {
    // COP1 BC1* branch family is valid only with COP1 enabled.
    if(!coprocessor1Enabled) return {~0ull, ~0ull};
    return {branchTargetVaddr, fallthroughVaddr};
  }
  return {~0ull, ~0ull};
}

// Build the static compilation plan before host code emission:
// - choose the linear window,
// - classify internal/external conditional edges,
// - collect internal JIT entry targets and alias addresses.
auto buildEmitPlan(ares::Nintendo64::CPU::Recompiler& recompiler, u64 vaddr, u32 address, bool singleInstruction,
                   std::vector<u32>& aliasAddresses) -> EmitPlan {
  EmitPlan plan = {};
  plan.startAddress = address;
  plan.startSection = recompiler.sectionIndex(address);
  plan.startVaddr = vaddr;
  plan.instructions.reserve(64);
  aliasAddresses.clear();

  Thread thread;
  {
    // First pass: grow a linear window until a hard stop condition.
    u64 currentVaddr = vaddr;
    u32 currentAddress = address;
    bool prevIsUncondBranch = false;
    while(true) {
      if(recompiler.sectionIndex(currentAddress) != plan.startSection) break;
      if(singleInstruction && !plan.instructions.empty()) break;
      u32 instruction = bus.read<Word>(currentAddress, thread, RBusDevice::ARES_JIT);
      auto info = recompiler.self.decoderEXECUTEInfo(instruction);
      // Materialize one prepass record per guest instruction.
      EmitPlannedInstruction pi;
      pi.vaddr = currentVaddr;
      pi.address = currentAddress;
      pi.instruction = instruction;
      pi.info = info;
      pi.countCompareWrite = info.countCompareWrite();
      pi.isUnconditional = info.unconditionalJump();
      if(info.branch()) {
        // Branch-like instructions get explicit taken/fallthrough targets.
        auto [taken, fallthrough] = computeBranchTargets(recompiler.emitStateKey.coprocessor1Enabled(), currentVaddr, instruction);
        pi.branchTakenVaddr = taken;
        pi.branchFallthroughVaddr = fallthrough;
      }
      plan.instructions.push_back(pi);
      // Stop one instruction after an unconditional jump to include its delay slot.
      if(prevIsUncondBranch) break;
      // Hard boundaries that must terminate the planning window.
      if(!info.branch() && (info.jitStateKeyMayChange() || pi.countCompareWrite)) break;
      if(recompiler.sectionIndex(currentAddress + 4) != plan.startSection) break;
      prevIsUncondBranch = pi.isUnconditional;
      currentVaddr += 4;
      currentAddress += 4;
    }
  }

  if(plan.instructions.empty()) return plan;
  plan.windowEndVaddr = plan.instructions.back().vaddr + 4;
  plan.windowEndAddress = plan.instructions.back().address + 4;

  // Delay-slot addresses are not legal direct entry points.
  std::vector<u64> validEntryVaddrs;
  validEntryVaddrs.reserve(plan.instructions.size());
  for(size_t i = 0; i < plan.instructions.size(); i++) {
    bool delaySlot = i > 0 && plan.instructions[i - 1].info.branch();
    if(!delaySlot) validEntryVaddrs.push_back(plan.instructions[i].vaddr);
  }
  auto isValidEntry = [&](u64 targetVaddr) -> bool {
    // Internal targets can never point into a delay slot.
    for(auto v : validEntryVaddrs) if(v == targetVaddr) return true;
    return false;
  };

  for(auto& instruction : plan.instructions) {
    // Second pass: classify each conditional edge as internal or external.
    if(!instruction.info.branch() || instruction.isUnconditional) continue;
    auto isInternal = [&](u64 targetVaddr) -> bool {
      // Reject null / out-of-window / unaligned candidate targets.
      if(targetVaddr == ~0ull) return false;
      if(targetVaddr < plan.startVaddr || targetVaddr >= plan.windowEndVaddr) return false;
      if((targetVaddr & 3) != 0) return false;
      if(!isValidEntry(targetVaddr)) return false;
      // Internal edges must stay cacheable and in the same 4 KiB section.
      auto access = recompiler.self.devirtualize<Read, Word>(targetVaddr, false, false);
      if(!access || !access.cache) return false;
      if(recompiler.sectionIndex(access.paddr) != plan.startSection) return false;
      return true;
    };
    instruction.branchTakenInternal = isInternal(instruction.branchTakenVaddr);
    instruction.branchFallthroughInternal = isInternal(instruction.branchFallthroughVaddr);
  }

  auto addInternalEntry = [&](u64 targetVaddr) {
    for(auto v : plan.internalEntryVaddrs) if(v == targetVaddr) return;
    plan.internalEntryVaddrs.push_back(targetVaddr);
  };
  auto addAliasAddress = [&](u32 targetAddress) {
    for(auto a : aliasAddresses) if(a == targetAddress) return;
    aliasAddresses.push_back(targetAddress);
  };

  for(auto& instruction : plan.instructions) {
    // Third pass: collect entry vaddrs for internal edges.
    if(!instruction.info.branch() || instruction.isUnconditional) continue;
    if(instruction.branchTakenInternal) addInternalEntry(instruction.branchTakenVaddr);
    if(instruction.branchFallthroughInternal) addInternalEntry(instruction.branchFallthroughVaddr);
  }
  for(auto targetVaddr : plan.internalEntryVaddrs) {
    // Publish physical addresses for alias block registration.
    auto access = recompiler.self.devirtualize<Read, Word>(targetVaddr, false, false);
    if(!access || !access.cache) continue;
    if(recompiler.sectionIndex(access.paddr) != plan.startSection) continue;
    addAliasAddress(access.paddr);
  }

  return plan;
}

}

auto CPU::Recompiler::emit(u64 vaddr, u32 address, u64 stateKey, bool singleInstruction) -> Block* {
  // Phase 0: initialize emit state and clear temporary outputs.
  emitStateKey = stateKey;
  emitSingleInstruction = singleInstruction;
  emitStateKeyChanged = false;
  emitAllocatorFlushed = false;
  emitAliasAddresses.clear();
  if(unlikely(allocator.available() < 1_MiB)) {
    print("CPU allocator flush\n");
    allocator.release();
    reset();
    emitAllocatorFlushed = true;
  }

  // Phase 1: reject impossible/non-coherent compilation targets.
  const u32 icacheTag = address & ~0xfffu;
  const u32 icacheBurstHi = icacheTag + 0xfe0u;
  const bool icacheBurstOk =
    icacheBurstHi <= 0x07ff'ffffu
    || (Model::Aleck64() && icacheTag > 0xbfff'ffffu && icacheBurstHi <= 0xc07f'ffffu);
  if(!icacheBurstOk) return nullptr;

  if(!self.icache.coherent(vaddr, address)) {
    // Fallback to interpreter path if the first line is already incoherent.
    return nullptr;
  }

  // Phase 2: prepass planning (window + edge classification + alias list).
  auto plan = buildEmitPlan(*this, vaddr, address, singleInstruction, emitAliasAddresses);
  if(plan.instructions.empty()) return nullptr;

  u32 startAddress = plan.startAddress;
  u32 startSection = plan.startSection;
  u32 windowEndAddress = plan.windowEndAddress;
  constexpr u32 branchToSelf = 0x1000'ffff;  //beq 0,0,<pc>

  // Internal entry labels are bound lazily when their instruction is emitted.
  std::vector<std::pair<u64, sljit_label*>> internalLabels;
  internalLabels.reserve(plan.internalEntryVaddrs.size());
  for(auto targetVaddr : plan.internalEntryVaddrs) {
    internalLabels.push_back({targetVaddr, nullptr});
  }
  auto findInternalLabel = [&](u64 targetVaddr) -> sljit_label** {
    for(auto& [vaddr, label] : internalLabels) {
      if(vaddr == targetVaddr) return &label;
    }
    return nullptr;
  };

  // Phase 3: begin host emission.
  beginFunction(3);
  slowPaths.clear();
  emitDeferredCycles = 0;

  // Pending forward jumps to internal labels not yet resolved (parallel vectors).
  std::vector<u64>          pendingJumpVaddrs;
  std::vector<sljit_jump*>  pendingJumpJumps;

  auto bindSlowPaths = [&](size_t first, sljit_label* resume, u32 deferredCycles, bool jumpEpilogFlag) -> void {
    // Convert deferred slow-path placeholders into concrete resumes.
    for(auto n = first; n < slowPaths.size(); n++) {
      auto& slow = slowPaths[n];
      if(slow.icacheMiss) continue;
      slow.resume = resume;
      slow.instructionCycles = deferredCycles - slow.deferredCycles;
      slow.jumpEpilog = jumpEpilogFlag;
    }
  };

  auto setLabelOrDefer = [&](sljit_jump* j, u64 targetVaddr) {
    // Forward internal jumps are patched when their label is emitted.
    auto* slot = findInternalLabel(targetVaddr);
    if(slot && *slot) { sljit_set_label(j, *slot); return; }
    pendingJumpVaddrs.push_back(targetVaddr);
    pendingJumpJumps.push_back(j);
  };

  auto emitInternalDispatch = [&](EmitPlannedInstruction& br) {
    cmp64(CpuClockMem, CpuJitClockTargetMem, set_uge);
    jumpEpilog(flag_uge);
    // This runs right after the branch delay slot.
    bool tInt = br.branchTakenInternal;
    bool fInt = br.branchFallthroughInternal;
    // No internal edge: return to dispatcher.
    if(!tInt && !fInt) { jumpEpilog(); return; }
    if(tInt && fInt) {
      // Both edges internal: choose taken/fallthrough from runtime pipeline PC.
      cmp64(PipelineReg(pc), imm(s64(br.branchTakenVaddr)), set_z);
      auto takenJ = jump(flag_z);
      cmp64(PipelineReg(pc), imm(s64(br.branchFallthroughVaddr)), set_z);
      auto fallJ = jump(flag_z);
      jumpEpilog();
      setLabelOrDefer(takenJ, br.branchTakenVaddr);
      setLabelOrDefer(fallJ, br.branchFallthroughVaddr);
      return;
    }
    if(tInt) {
      // Taken internal, fallthrough external.
      cmp64(PipelineReg(pc), imm(s64(br.branchTakenVaddr)), set_z);
      auto takenJ = jump(flag_z);
      jumpEpilog();
      setLabelOrDefer(takenJ, br.branchTakenVaddr);
      return;
    }
    // Fallthrough internal, taken external.
    cmp64(PipelineReg(pc), imm(s64(br.branchFallthroughVaddr)), set_z);
    auto fJ = jump(flag_z);
    jumpEpilog();
    setLabelOrDefer(fJ, br.branchFallthroughVaddr);
  };

  for(auto& [targetVaddr, targetLabel] : internalLabels) {
    (void)targetLabel;
    // Entry dispatcher allows external lookup to jump into internal targets.
    cmp64(mem(IpuReg(pc)), imm(s64(targetVaddr)), set_z);
    auto entryJump = jump(flag_z);
    setLabelOrDefer(entryJump, targetVaddr);
  }

  // Phase 4: emit the planned instruction sequence.
  bool prevBranched = false;
  for(size_t idx = 0; idx < plan.instructions.size(); idx++) {
    auto& ii = plan.instructions[idx];
    bool firstInstruction = (idx == 0);
    bool delaySlot = prevBranched;
    bool isInternalEntry = findInternalLabel(ii.vaddr) != nullptr;
    bool sectionBoundary = sectionIndex(ii.address + 4) != startSection;
    u32 instruction = ii.instruction;
    OpInfo info = ii.info;
    bool countCompareWrite = ii.countCompareWrite;

    // Runtime PC mode is needed at block entry and in delay slots.
    emitVaddr = ii.vaddr;
    emitPcMode = (delaySlot || firstInstruction) ? EmitPcMode::Runtime : EmitPcMode::JitTime;
    emitPipelineSetupDone = false;
    emitCallfSetupDone = false;
    emitCallfEmitted = false;

    if(callInstructionPrologue) {
      // Optional debugger/profiler instruction hook.
      flushDeferredCycles();
      callf(&CPU::instructionPrologue, imm64(ii.vaddr), imm(instruction));
    }

    if(isInternalEntry) {
      // Resolve this internal entry and patch all pending forward jumps.
      flushDeferredCycles();
      auto lbl = sljit_emit_label(compiler);
      *findInternalLabel(ii.vaddr) = lbl;
      for(size_t k = 0; k < pendingJumpVaddrs.size();) {
        if(pendingJumpVaddrs[k] == ii.vaddr) {
          sljit_set_label(pendingJumpJumps[k], lbl);
          pendingJumpVaddrs[k] = pendingJumpVaddrs.back(); pendingJumpVaddrs.pop_back();
          pendingJumpJumps[k]  = pendingJumpJumps.back();  pendingJumpJumps.pop_back();
        } else {
          k++;
        }
      }
    }

    if(firstInstruction || (ii.vaddr & 0x1f) == 0) {
      // Keep icache tag/coherency checks in sync with interpreter behavior.
      flushDeferredCycles();
      if(!self.icache.coherent(ii.vaddr, ii.address)) {
        resetCompiler();
        return nullptr;
      }
      const u32 lineIndex = u32(ii.vaddr >> 5) & 0x1ffu;
      const u32 expectedTagKey = (ii.address & ~0xfffu) | 1u;
      cmp32(IcacheTagKeyMem(lineIndex), imm(expectedTagKey), set_z);
      auto icacheMiss = jump(flag_ne);
      if(system.homebrewMode) {
        add64(ProfileIcacheHitsMem, ProfileIcacheHitsMem, imm(1));
      }
      deferSlowPathCacheMiss(icacheMiss, ii.address);
    }

    auto slowPathStart = slowPaths.size();
    // Branch emitters require a ready pipeline window.
    if(info.branch()) setupPipeline();
    auto emitResult = emitEXECUTE(instruction, false, emitPcMode);
    bool branched = emitResult == EmitExecuteResult::MayBranch;
    u32 instructionCycles = 1 * 2;
    u32 jumpToSelf = 2 << 26 | u32(ii.vaddr >> 2 & 0x3ff'ffff);
    if(unlikely(instruction == branchToSelf || instruction == jumpToSelf)) {
      instructionCycles = 64 * 2;
    }
    emitDeferredCycles += instructionCycles;
    // Synchronize cycles before any branch/helper boundary.
    if(delaySlot || info.branch() || emitCallfEmitted) flushDeferredCycles();

    // EndBlock must be observed at entry, delay-slot completion, likely-branch flows and helpers.
    bool needEndBlockCheck = firstInstruction || delaySlot || info.likelyBranch() || emitCallfEmitted;
    // Commit architectural pipeline state only where correctness requires it.
    bool needPipelineCommit = needEndBlockCheck || info.branch() || sectionBoundary
                            || singleInstruction || info.jitStateKeyMayChange()
                            || countCompareWrite;
    if(needPipelineCommit) setupPipeline();
    if(needEndBlockCheck) {
      test32(PipelineReg(state), imm(Pipeline::EndBlock), set_z);
    }
    if(needPipelineCommit) {
      mov64(mem(IpuReg(pc)), PipelineReg(pc));
      mov32(PipelineReg(state), PipelineReg(nstate));
    }
    if(needEndBlockCheck) {
      jumpEpilog(flag_nz);
    }

    if(slowPaths.size() != slowPathStart) {
      // New slow paths created by this opcode resume right after it.
      auto resume = sljit_emit_label(compiler);
      bindSlowPaths(slowPathStart, resume, emitDeferredCycles, !delaySlot);
    }

    bool prevIsConditionalBranch = delaySlot && idx > 0
      && plan.instructions[idx - 1].info.branch() && !plan.instructions[idx - 1].isUnconditional;
    if(prevIsConditionalBranch) {
      // Conditional branch dispatch happens after its delay slot.
      emitInternalDispatch(plan.instructions[idx - 1]);
    }
    prevBranched = branched;
  }

  // Phase 5: emit epilogue and deferred slow paths.
  flushDeferredCycles();
  jumpEpilog();
  for(auto& slow : slowPaths) {
    // Every deferred slow path gets a dedicated entry trampoline.
    auto enter = sljit_emit_label(compiler);
    for(auto jump : slow.enters) sljit_set_label(jump, enter);
    emitVaddr = slow.vaddr;
    emitDeferredCycles = slow.deferredCycles;
    emitCallfSetupDone = false;
    emitCallfEmitted = false;
    if(slow.icacheMiss) {
      // I-cache refill slow path.
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
      // Generic opcode slow path.
      emitPcMode = slow.runtimePc ? EmitPcMode::Runtime : EmitPcMode::JitTime;
      emitPipelineSetupDone = false;
      OpInfo info = self.decoderEXECUTEInfo(slow.instruction);
      if(info.branch()) setupPipeline();
      emitEXECUTE(slow.instruction, true, emitPcMode);
      emitDeferredCycles += slow.instructionCycles;
      flushDeferredCycles();
      if(!emitPipelineSetupDone) setupPipeline();
      test32(PipelineReg(state), imm(Pipeline::EndBlock), set_z);
      mov32(PipelineReg(state), PipelineReg(nstate));
      mov64(mem(IpuReg(pc)), PipelineReg(pc));
      /*if(slow.jumpEpilog)*/ jumpEpilog(flag_nz);
    }
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), slow.resume);
  }

  memory::jitprotect(false);
  // Phase 6: publish block metadata.
  auto block = (Block*)allocator.acquire(sizeof(Block));
  block->code = endFunction();
  block->next = nullptr;
  block->stateKey = stateKey;
  block->vaddrPage = plan.startVaddr & ~0xfffull;
  block->startAddress = startAddress;
  block->endAddress = windowEndAddress;
  block->sectionDirty = sectionDirty.data() + startSection;

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
  if(emitSlowPath || emitSingleInstruction || (require64 && reservedInstruction64())
  || ((partialLeft || partialRight) && emitStateKey.reverseEndian())
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
      if(partialLeft && size == Word) {
        and32(reg(3), reg(0), imm(3));
        mov32(reg(1), imm(0x0f));
        lshr32(reg(1), reg(1), reg(3));
        and32(reg(3), reg(0), imm(0x0c));
        shl32(reg(1), reg(1), reg(3));
      } else if(partialRight && size == Word) {
        and32(reg(3), reg(0), imm(3));
        add32(reg(3), reg(3), imm(1));
        mov32(reg(1), imm(1));
        shl32(reg(1), reg(1), reg(3));
        sub32(reg(1), reg(1), imm(1));
        and32(reg(3), reg(0), imm(0x0c));
        shl32(reg(1), reg(1), reg(3));
      } else {
        and32(reg(3), reg(0), imm(0x0f));
        mov32(reg(1), imm((1 << size) - 1));
        shl32(reg(1), reg(1), reg(3));
      }
    }

    if(floating && size == Word) {
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
      xor32(reg(4), reg(4), imm(3));
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
      xor32(reg(1), reg(1), imm(7));
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
      mov32_u16(reg(0), mem(reg(2), DcacheLineDirtyOff));
      or32(reg(1), reg(1), reg(0));
      mov32_u16(mem(reg(2), DcacheLineDirtyOff), reg(1));
      mov64(mem(reg(2), DcacheLineDirtyPcOff), imm(emitVaddr));
    } else {
      mov32_u16(mem(reg(2), DcacheLineDirtyOff), imm(1));
    }
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
      xor32(reg(1), reg(1), imm(3));
      shl32(reg(1), reg(1), imm(3));
      lshr32(reg(3), reg(3), reg(1));
      mov32(reg(2), imm((sljit_sw)0xffff'ffffu));
      lshr32(reg(0), reg(2), reg(1));
      xor32(reg(0), reg(0), imm((sljit_sw)0xffff'ffffu));
      and32(reg(0), mem(Rt32), reg(0));
      or32(reg(3), reg(3), reg(0));
      if(extendedAddressing) mov64_s32(reg(3), reg(3));
      else                   mov64_u32(reg(3), reg(3));
    } else if(partialRight && size == Dual) {
      and32(reg(1), reg(0), imm(7));
      xor32(reg(1), reg(1), imm(7));
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
    cmp64(mem(Rs), mem(Rt), set_z);
    auto notTaken = jump(flag_nz);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    cmp64(mem(Rs), mem(Rt), set_z);
    auto taken = jump(flag_nz);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BLEZ Rs,i16
  case 0x06: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto notTaken = jump(flag_sgt);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BGTZ Rs,i16
  case 0x07: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto taken = jump(flag_sgt);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
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
    cmp64(mem(Rs), mem(Rt), set_z);
    auto taken = jump(flag_z);
    emitLikelyNotTaken();
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BNEL Rs,Rt,i16
  case 0x15: {
    cmp64(mem(Rs), mem(Rt), set_z);
    auto notTaken = jump(flag_z);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    emitLikelyNotTaken();
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BLEZL Rs,i16
  case 0x16: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto notTaken = jump(flag_sgt);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    emitLikelyNotTaken();
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BGTZL Rs,i16
  case 0x17: {
    cmp64(mem(Rs), imm(0), set_sgt);
    auto taken = jump(flag_sgt);
    emitLikelyNotTaken();
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
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
    setupCallf();
    callf(&CPU::LL, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
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
    setupCallf();
    callf(&CPU::LLD, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
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
    setupCallf();
    callf(&CPU::SC, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
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
    setupCallf();
    callf(&CPU::SCD, mem(Rt), mem(Rs), imm(i16));
    emitZeroClear(Rtn);
    return EmitExecuteResult::MayFault;
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
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
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
    bool reservedInstruction = reservedInstruction64();
    if(emitSlowPathSection || reservedInstruction) {
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
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BGEZ Rs,i16
  case 0x01: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BLTZL Rs,i16
  case 0x02: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    emitLikelyNotTaken();
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BGEZL Rs,i16
  case 0x03: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    emitLikelyNotTaken();
    setLabel(done);
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
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot));
    setLabel(done);
    emitLink31();
    return EmitExecuteResult::MayBranch;
  }

  //BLTZALL Rs,i16
  case 0x12: {
    emitLink31();
    cmp64(mem(Rs), imm(0), set_slt);
    auto taken = jump(flag_slt);
    emitLikelyNotTaken();
    auto done = jump();
    setLabel(taken);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    setLabel(done);
    return EmitExecuteResult::MayBranch;
  }

  //BGEZALL Rs,i16
  case 0x13: {
    emitLink31();
    cmp64(mem(Rs), imm(0), set_slt);
    auto notTaken = jump(flag_slt);
    emitBranchTarget(i16);
    mov32(PipelineReg(nstate), imm(Pipeline::DelaySlot | Pipeline::EndBlock));
    auto done = jump();
    setLabel(notTaken);
    emitLikelyNotTaken();
    setLabel(done);
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
