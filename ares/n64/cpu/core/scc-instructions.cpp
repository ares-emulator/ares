auto CPU::instructionDMFC0() -> void {
  RT.u64 = getControlRegister(RDn);
}

auto CPU::instructionDMTC0() -> void {
  setControlRegister(RDn, RT.u64);
}

auto CPU::instructionERET() -> void {
  branch.exception();
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
  setControlRegister(RDn, i32(RT.u32));
}

auto CPU::instructionTLBP() -> void {
  for(uint index : range(TLB::Entries)) {
    auto& entry = tlb.entry[index];
    if(!entry.global[0] || !entry.global[1]) {
      if(entry.addressSpaceID != scc.tlb.addressSpaceID) continue;
    }
    if(entry.virtualAddress != scc.tlb.virtualAddress) continue;
    if(entry.region != scc.tlb.region) continue;
    scc.index.tlbEntry = index;
    scc.index.probeFailure = 0;
    return;
  }
  scc.index.probeFailure = 1;
}

auto CPU::instructionTLBR() -> void {
  if(scc.index.tlbEntry >= TLB::Entries) return;
  scc.tlb = tlb.entry[scc.index.tlbEntry];
}

auto CPU::instructionTLBWI() -> void {
  if(scc.index.tlbEntry >= TLB::Entries) return;
  tlb.entry[scc.index.tlbEntry] = scc.tlb;
}

auto CPU::instructionTLBWR() -> void {
  if(scc.random.index >= TLB::Entries) return;
  tlb.entry[scc.random.index] = scc.tlb;
}
