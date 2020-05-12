auto CPU::instructionBC0() -> void {
  bool condition = OP >> 16 & 1;
  bool likely    = OP >> 17 & 1;
  if(scc.cf == condition) IP = PC + (IMMi16 << 2);
  else if(likely) PC += 4;
}

auto CPU::instructionCFC0() -> void {
  RT.u64 = i32(scc.cr[RDn]);
}

auto CPU::instructionCTC0() -> void {
  scc.cr[RDn] = RT.u32;
}

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
}

auto CPU::instructionMFC0() -> void {
  RT.u64 = i32(getControlRegister(RDn));
}

auto CPU::instructionMTC0() -> void {
  setControlRegister(RDn, RT.u32);
}

auto CPU::instructionTLBP() -> void {
  for(uint index : range(TLB::Entries)) {
    auto& tlb = this->tlb[index];
    if(tlb.addressSpaceID != scc.entryHi.addressSpaceID && !tlb.global) continue;
    if(tlb.virtualAddress != scc.entryHi.virtualAddress) continue;
    if(tlb.unused != scc.entryHi.unused) continue;
    if(tlb.region != scc.entryHi.region) continue;
    scc.index.tlbEntry = index;
    scc.index.probeFailure = 0;
    return;
  }
  scc.index.probeFailure = 1;
}

auto CPU::instructionTLBR() -> void {
  if(scc.index.tlbEntry >= TLB::Entries) return;
  auto& tlb = this->tlb[scc.index.tlbEntry];
  for(uint n : range(2)) {
    scc.entryLo[n].pageValid = tlb.pageValid[n];
    scc.entryLo[n].pageDirty = tlb.pageDirty[n];
    scc.entryLo[n].cacheAlgorithm = tlb.cacheAlgorithm[n];
    scc.entryLo[n].physicalAddress = tlb.physicalAddress[n];
    scc.entryLo[n].global = tlb.global;
  }
  scc.entryHi.addressSpaceID = tlb.addressSpaceID;
  scc.entryHi.virtualAddress = tlb.virtualAddress;
  scc.entryHi.unused = tlb.unused;
  scc.entryHi.region = tlb.region;
  scc.pageMask = tlb.pageMask;
}

auto CPU::instructionTLBWI() -> void {
  if(scc.index.tlbEntry >= TLB::Entries) return;
  auto& tlb = this->tlb[scc.index.tlbEntry];
  for(uint n : range(2)) {
    tlb.pageValid[n] = scc.entryLo[n].pageValid;
    tlb.pageDirty[n] = scc.entryLo[n].pageDirty;
    tlb.cacheAlgorithm[n] = scc.entryLo[n].cacheAlgorithm;
    tlb.physicalAddress[n] = scc.entryLo[n].physicalAddress;
  }
  tlb.global = scc.entryLo[0].global && scc.entryLo[1].global;
  tlb.addressSpaceID = scc.entryHi.addressSpaceID;
  tlb.virtualAddress = scc.entryHi.virtualAddress;
  tlb.unused = scc.entryHi.unused;
  tlb.region = scc.entryHi.region;
  tlb.pageMask = scc.pageMask;
}

auto CPU::instructionTLBWR() -> void {
  if(scc.index.tlbEntry >= TLB::Entries) return;
  auto& tlb = this->tlb[scc.random.index];
  for(uint n : range(2)) {
    tlb.pageValid[n] = scc.entryLo[n].pageValid;
    tlb.pageDirty[n] = scc.entryLo[n].pageDirty;
    tlb.cacheAlgorithm[n] = scc.entryLo[n].cacheAlgorithm;
    tlb.physicalAddress[n] = scc.entryLo[n].physicalAddress;
  }
  tlb.global = scc.entryLo[0].global && scc.entryLo[1].global;
  tlb.addressSpaceID = scc.entryHi.addressSpaceID;
  tlb.virtualAddress = scc.entryHi.virtualAddress;
  tlb.unused = scc.entryHi.unused;
  tlb.region = scc.entryHi.region;
  tlb.pageMask = scc.pageMask;
}
