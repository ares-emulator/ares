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
   - block(vaddr, paddr) probes a per-section cache.
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
  section boundary, non-branch stateKey-changing ops, SCC Count/Compare writes,
  breakpoint barriers, and delay-slot completion after unconditional jumps.

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
  stateKey.setWatchpointsActive(GDB::server.hasWatchpoints());
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

auto CPU::Recompiler::block(u64 vaddr, u32 address) -> Block* {
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

  auto block = emit(vaddr, address, stateKey);
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

auto endsBlockAfterDelaySlot(ares::Nintendo64::CPU::OpInfo info) -> bool {
  if(!info.unconditionalJump()) return false;
  if(info.unconditionalJumpAndLink()) return false;
  return true;
}

// Build the static compilation plan before host code emission:
// - choose the linear window,
// - classify internal/external conditional edges,
// - collect internal JIT entry targets and alias addresses.
auto buildEmitPlan(ares::Nintendo64::CPU::Recompiler& recompiler, u64 vaddr, u32 address,
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
    bool prevEndsBlockAfterDelaySlot = false;
    while(true) {
      if(recompiler.sectionIndex(currentAddress) != plan.startSection) break;
      if(!plan.instructions.empty() && GDB::server.hasBreakpointAt(u32(currentVaddr))) break;
      u32 instruction = bus.read<Word>(currentAddress, thread, RBusDevice::ARES_JIT);
      auto info = recompiler.self.decoderEXECUTEInfo(instruction);
      // Materialize one prepass record per guest instruction.
      EmitPlannedInstruction pi;
      pi.vaddr = currentVaddr;
      pi.address = currentAddress;
      pi.instruction = instruction;
      pi.info = info;
      plan.instructions.push_back(pi);
      if(prevEndsBlockAfterDelaySlot) break;
      // Hard boundaries that must terminate the planning window.
      if(!info.branch() && (info.jitStateKeyMayChange() || info.countCompareWrite())) break;
      if(recompiler.sectionIndex(currentAddress + 4) != plan.startSection) break;
      prevEndsBlockAfterDelaySlot = endsBlockAfterDelaySlot(info);
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

  auto addInternalEntry = [&](u64 targetVaddr) {
    for(auto v : plan.internalEntryVaddrs) if(v == targetVaddr) return;
    plan.internalEntryVaddrs.push_back(targetVaddr);
  };

  for(auto& instruction : plan.instructions) {
    auto isInternal = [&](u64 targetVaddr) -> bool {
      // Reject null / out-of-window / unaligned candidate targets.
      if(targetVaddr == ~0ull) return false;
      if(targetVaddr < plan.startVaddr || targetVaddr >= plan.windowEndVaddr) return false;
      if((targetVaddr & 3) != 0) return false;
      if(!isValidEntry(targetVaddr)) return false;
      if(GDB::server.hasBreakpointAt(u32(targetVaddr))) return false;
      u32 targetAddress = plan.startAddress + u32(targetVaddr - plan.startVaddr);
      if(recompiler.sectionIndex(targetAddress) != plan.startSection) return false;
      return true;
    };
    if(!instruction.info.branch()) continue;
    if(instruction.info.unconditionalJump()) {
      if(!instruction.info.unconditionalJumpAndLink()) continue;
      u64 returnVaddr = instruction.vaddr + 8;
      if(isInternal(returnVaddr)) addInternalEntry(returnVaddr);
      continue;
    }
    // Second pass: classify each conditional edge as internal or external.
    auto [branchTakenVaddr, branchFallthroughVaddr] =
      computeBranchTargets(recompiler.emitStateKey.coprocessor1Enabled(), instruction.vaddr, instruction.instruction);
    if(isInternal(branchTakenVaddr)) addInternalEntry(branchTakenVaddr);
    if(isInternal(branchFallthroughVaddr)) addInternalEntry(branchFallthroughVaddr);
  }

  auto addAliasAddress = [&](u32 targetAddress) {
    for(auto a : aliasAddresses) if(a == targetAddress) return;
    aliasAddresses.push_back(targetAddress);
  };
  for(auto targetVaddr : plan.internalEntryVaddrs) {
    u32 targetAddress = plan.startAddress + u32(targetVaddr - plan.startVaddr);
    if(recompiler.sectionIndex(targetAddress) != plan.startSection) continue;
    addAliasAddress(targetAddress);
  }

  return plan;
}

}

auto CPU::Recompiler::emit(u64 vaddr, u32 address, u64 stateKey) -> Block* {
  // Phase 0: initialize emit state and clear temporary outputs.
  emitStateKey = stateKey;
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
  auto plan = buildEmitPlan(*this, vaddr, address, emitAliasAddresses);
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
    auto [branchTakenVaddr, branchFallthroughVaddr] =
      computeBranchTargets(emitStateKey.coprocessor1Enabled(), br.vaddr, br.instruction);
    bool tInt = false;
    bool fInt = false;
    for(auto target : plan.internalEntryVaddrs) {
      if(target == branchTakenVaddr) tInt = true;
      if(target == branchFallthroughVaddr) fInt = true;
    }
    // This runs right after the branch delay slot.
    // No internal edge: return to dispatcher.
    if(!tInt && !fInt) { jumpEpilog(); return; }
    if(tInt && fInt) {
      // Both edges internal: choose taken/fallthrough from runtime pipeline PC.
      cmp64(PipelineReg(pc), imm(s64(branchTakenVaddr)), set_z);
      auto takenJ = jump(flag_z);
      cmp64(PipelineReg(pc), imm(s64(branchFallthroughVaddr)), set_z);
      auto fallJ = jump(flag_z);
      jumpEpilog();
      setLabelOrDefer(takenJ, branchTakenVaddr);
      setLabelOrDefer(fallJ, branchFallthroughVaddr);
      return;
    }
    if(tInt) {
      // Taken internal, fallthrough external.
      cmp64(PipelineReg(pc), imm(s64(branchTakenVaddr)), set_z);
      auto takenJ = jump(flag_z);
      jumpEpilog();
      setLabelOrDefer(takenJ, branchTakenVaddr);
      return;
    }
    // Fallthrough internal, taken external.
    cmp64(PipelineReg(pc), imm(s64(branchFallthroughVaddr)), set_z);
    auto fJ = jump(flag_z);
    jumpEpilog();
    setLabelOrDefer(fJ, branchFallthroughVaddr);
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
                            || info.jitStateKeyMayChange()
                            || info.countCompareWrite();
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
      && plan.instructions[idx - 1].info.branch() && !plan.instructions[idx - 1].info.unconditionalJump();
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
