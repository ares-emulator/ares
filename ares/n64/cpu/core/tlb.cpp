auto CPU::tlbLookup(u64 address) -> u64 {
  if constexpr(Accuracy::TLB) {
    for(uint index : range(TLB::Entries)) {
      auto& tlb = this->tlb[index];
      if(!tlb.pageValid) continue;
      u64 mask = u64(0) - 1 - tlb.pageMask & 0x1fff'ffff;
      if((address & mask) == (tlb.virtualAddress & mask)) {
        if(tlb.global || (address >> 32 & 255) == tlb.addressSpaceID) {
          bool n = address & 1;
          address = (address & 0xe000'0000) | (tlb.physicalAddress[n] & mask) | (address & tlb.pageMask);
          break;
        }
      }
    }
  }
  return address;
}
