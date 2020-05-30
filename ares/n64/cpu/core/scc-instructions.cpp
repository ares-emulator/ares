auto CPU::instructionDMFC0(r64& rt, u8 rd) -> void {
  rt.u64 = getControlRegister(rd);
}

auto CPU::instructionDMTC0(cr64& rt, u8 rd) -> void {
  setControlRegister(rd, rt.u64);
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

auto CPU::instructionMFC0(r64& rt, u8 rd) -> void {
  rt.u64 = i32(getControlRegister(rd));
}

auto CPU::instructionMTC0(cr64& rt, u8 rd) -> void {
  setControlRegister(rd, i32(rt.u32));
}

auto CPU::instructionTLBP() -> void {
  scc.index.tlbEntry = 0;  //technically undefined
  scc.index.probeFailure = 1;
  for(uint index : range(TLB::Entries)) {
    auto& entry = tlb.entry[index];
    auto mask = ~entry.pageMask & ~0x1fff;
    if((entry.virtualAddress & mask) != (scc.tlb.virtualAddress & mask)) continue;
    if(!entry.global[0] || !entry.global[1]) {
      if(entry.addressSpaceID != scc.tlb.addressSpaceID) continue;
    }
    scc.index.tlbEntry = index;
    scc.index.probeFailure = 0;
    break;
  }
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
  tlb.entry[scc.index.tlbEntry] = scc.tlb;
}
