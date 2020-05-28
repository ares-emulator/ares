auto CPU::TLB::load(u32 address) -> maybe<u32> {
  for(auto& entry : this->entry) {
    if(!entry.globals) continue;
    if((address & entry.addressMaskHi) != entry.addressCompare) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) return self.exception.tlbLoad(), nothing;
    return physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
  }
  return self.exception.tlbLoad(), nothing;
}

auto CPU::TLB::store(u32 address) -> maybe<u32> {
  for(auto& entry : this->entry) {
    if(!entry.globals) continue;
    if((address & entry.addressMaskHi) != entry.addressCompare) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) return self.exception.tlbStore(), nothing;
    if(!entry.dirty[lo]) return self.exception.tlbModification(), nothing;
    return physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
  }
  return self.exception.tlbStore(), nothing;
}

auto CPU::TLB::exception(u32 address) -> void {
  self.scc.badVirtualAddress = address;
  self.scc.tlb.addressSpaceID = 0;  //address >> 32;  //note: needs u64 address
  self.scc.tlb.virtualAddress = address >> 13;
  self.scc.context.badVirtualAddress = address >> 13;
  self.scc.xcontext.badVirtualAddress = address >> 13;
}

auto CPU::TLB::Entry::synchronize() -> void {
  globals = global[0] && global[1];
  addressMaskHi = ~(pageMask | 0x1fff);
  addressMaskLo = (pageMask | 0x1fff) >> 1;
  addressSelect = addressMaskLo + 1;
  addressCompare = virtualAddress & addressMaskHi;
}
