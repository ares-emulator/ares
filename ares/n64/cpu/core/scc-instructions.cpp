auto CPU::instructionDMFC0() -> void {
  RT.u64 = getControlRegister(RDn);
}

auto CPU::instructionDMTC0() -> void {
  setControlRegister(RDn, RT.u64);
}

auto CPU::instructionERET() -> void {
  if(scc.status.errorLevel) {
    PC = scc.epcError;
    scc.status.errorLevel = 0;
  } else {
    PC = scc.epc;
    scc.status.exceptionLevel = 0;
  }
  scc.llbit = 0;
  context.setMode();
}

auto CPU::instructionMFC0() -> void {
  RT.u64 = i32(getControlRegister(RDn));
}

auto CPU::instructionMTC0() -> void {
  setControlRegister(RDn, RT.u32);
}

auto CPU::instructionTLBP() -> void {
  for(uint index : range(TLB::Entries)) {
    auto& entry = tlb.entry[index << 1];
    if(entry.addressSpaceID != scc.entryHi.addressSpaceID && !entry.global) continue;
    if(entry.virtualAddress != scc.entryHi.virtualAddress) continue;
    if(entry.region != scc.entryHi.region) continue;
    scc.index.tlbEntry = index >> 1;
    scc.index.probeFailure = 0;
    return;
  }
  scc.index.probeFailure = 1;
}

auto CPU::instructionTLBR() -> void {
  if(scc.index.tlbEntry >= TLB::Entries) return;
  auto& lo = tlb.entry[scc.index.tlbEntry << 1 | 0];
  auto& hi = tlb.entry[scc.index.tlbEntry << 1 | 1];
  for(uint n : range(2)) {
    scc.entryLo[0].pageValid = lo.valid;
    scc.entryLo[1].pageValid = hi.valid;
    scc.entryLo[0].pageDirty = lo.dirty;
    scc.entryLo[1].pageDirty = hi.dirty;
    scc.entryLo[0].cacheAlgorithm = lo.cached ? 0b011 : 0b010;
    scc.entryLo[1].cacheAlgorithm = hi.cached ? 0b011 : 0b010;
    scc.entryLo[0].global = lo.global;
    scc.entryLo[1].global = hi.global;
    scc.entryLo[0].physicalAddress = lo.physicalAddress >> 6;
    scc.entryLo[1].physicalAddress = hi.physicalAddress >> 6;
  }
  scc.entryHi.virtualAddress = lo.virtualAddress >> 13;
  scc.entryHi.addressSpaceID = lo.addressSpaceID;
  scc.entryHi.region = lo.region;
  scc.pageMask = lo.mask >> 13;
}

auto CPU::instructionTLBWI() -> void {
  if(scc.index.tlbEntry >= TLB::Entries) return;
  auto& lo = tlb.entry[scc.index.tlbEntry << 1 | 0];
  auto& hi = tlb.entry[scc.index.tlbEntry << 1 | 1];
  for(uint n : range(2)) {
    lo.valid = scc.entryLo[0].pageValid;
    hi.valid = scc.entryLo[1].pageValid;
    lo.dirty = scc.entryLo[0].pageDirty;
    hi.dirty = scc.entryLo[1].pageDirty;
    lo.cached = scc.entryLo[0].cacheAlgorithm != 0b010;
    hi.cached = scc.entryLo[1].cacheAlgorithm != 0b010;
    lo.global = scc.entryLo[0].global;
    hi.global = scc.entryLo[1].global;
    lo.physicalAddress = scc.entryLo[0].physicalAddress << 6;
    hi.physicalAddress = scc.entryLo[1].physicalAddress << 6;
  }
  lo.virtualAddress = scc.entryHi.virtualAddress << 13;
  hi.virtualAddress = scc.entryHi.virtualAddress << 13;
  lo.addressSpaceID = scc.entryHi.addressSpaceID;
  hi.addressSpaceID = scc.entryHi.addressSpaceID;
  lo.region = scc.entryHi.region;
  hi.region = scc.entryHi.region;
  lo.mask = scc.pageMask << 13;
  hi.mask = scc.pageMask << 13;
  tlb.rebuild();
}

auto CPU::instructionTLBWR() -> void {
  if(scc.random.index >= TLB::Entries) return;
  auto& lo = tlb.entry[scc.random.index << 1 | 0];
  auto& hi = tlb.entry[scc.random.index << 1 | 1];
  for(uint n : range(2)) {
    lo.valid = scc.entryLo[0].pageValid;
    hi.valid = scc.entryLo[1].pageValid;
    lo.dirty = scc.entryLo[0].pageDirty;
    hi.dirty = scc.entryLo[1].pageDirty;
    lo.cached = scc.entryLo[0].cacheAlgorithm != 0b010;
    hi.cached = scc.entryLo[1].cacheAlgorithm != 0b010;
    lo.global = scc.entryLo[0].global;
    hi.global = scc.entryLo[1].global;
    lo.physicalAddress = scc.entryLo[0].physicalAddress << 6;
    hi.physicalAddress = scc.entryLo[1].physicalAddress << 6;
  }
  lo.virtualAddress = scc.entryHi.virtualAddress << 13;
  hi.virtualAddress = scc.entryHi.virtualAddress << 13;
  lo.addressSpaceID = scc.entryHi.addressSpaceID;
  hi.addressSpaceID = scc.entryHi.addressSpaceID;
  lo.region = scc.entryHi.region;
  hi.region = scc.entryHi.region;
  lo.mask = scc.pageMask << 13;
  hi.mask = scc.pageMask << 13;
  tlb.rebuild();
}
