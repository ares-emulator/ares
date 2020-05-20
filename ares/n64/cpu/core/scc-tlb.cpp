auto CPU::TLB::load(u32 address) -> maybe<u32> {
  for(uint index : range(TLB::Entries)) {
    if((address & ~entry[index].pageMask) != entry[index].virtualAddress) continue;
    auto lo = bool(address & entry[index].pageMask + 1);
    if(!entry[index].valid[lo]) return self.exception.tlbLoad(), nothing;
    return physicalAddress = entry[index].physicalAddress[lo] + (address & entry[index].pageMask);
  }
  return self.exception.tlbLoad(), nothing;
}

auto CPU::TLB::store(u32 address) -> maybe<u32> {
  for(uint index : range(TLB::Entries)) {
    if((address & ~entry[index].pageMask) != entry[index].virtualAddress) continue;
    auto lo = bool(address & entry[index].pageMask + 1);
    if(!entry[index].valid[lo]) return self.exception.tlbStore(), nothing;
    if(!entry[index].dirty[lo]) return self.exception.tlbModification(), nothing;
    return physicalAddress = entry[index].physicalAddress[lo] + (address & entry[index].pageMask);
  }
  return self.exception.tlbStore(), nothing;
}

auto CPU::TLB::exception(u32 address) -> void {
  self.scc.badVirtualAddress = address;
  self.scc.tlb.addressSpaceID = address >> 32;  //note: needs u64 address
  self.scc.tlb.virtualAddress = address >> 13;
  self.scc.context.badVirtualAddress = address >> 13;
  self.scc.xcontext.badVirtualAddress = address >> 13;
}
