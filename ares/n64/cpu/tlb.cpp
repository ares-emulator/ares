
auto CPU::TLB::load(u64 vaddr, const Entry& entry, bool noExceptions) -> maybe<PhysAccess> {
  if(!entry.globals && entry.addressSpaceID != self.scc.tlb.addressSpaceID) return nothing;
  if((vaddr & entry.addressMaskHi) != entry.virtualAddress) return nothing;
  if(vaddr >> 62 != entry.region) return nothing;
  bool lo = vaddr & entry.addressSelect;
  if(!entry.valid[lo]) {
    if(noExceptions)return PhysAccess{false};
    
    self.addressException(vaddr);
    self.debugger.tlbLoadInvalid(vaddr);
    self.exception.tlbLoadInvalid();
    return PhysAccess{false};
  }
  physicalAddress = entry.physicalAddress[lo] + (vaddr & entry.addressMaskLo);
  self.debugger.tlbLoad(vaddr, physicalAddress);
  return PhysAccess{true, entry.cacheAlgorithm[lo] != 2, physicalAddress, vaddr};
}

auto CPU::TLB::load(u64 vaddr, bool noExceptions) -> PhysAccess {
  for(auto& entry : this->tlbCache.entry) {
    if(!entry.entry) continue;
    if(auto match = load(vaddr, *entry.entry, noExceptions)) {
      entry.frequency++;
      return *match;
    }
  }

  for(auto& entry : this->entry) {
    if(auto match = load(vaddr, entry, noExceptions)) {
      this->tlbCache.insert(entry);
      return *match;
    }
  }

  if(noExceptions)return {false};

  self.addressException(vaddr);
  self.debugger.tlbLoadMiss(vaddr);
  self.exception.tlbLoadMiss();
  return {false};
}

auto CPU::TLB::store(u64 vaddr, const Entry& entry, bool noExceptions) -> maybe<PhysAccess> {
  if(!entry.globals && entry.addressSpaceID != self.scc.tlb.addressSpaceID) return nothing;
  if((vaddr & entry.addressMaskHi) != entry.virtualAddress) return nothing;
  if(vaddr >> 62 != entry.region) return nothing;
  bool lo = vaddr & entry.addressSelect;
  if(!entry.valid[lo]) {
    if(!noExceptions) {
      self.addressException(vaddr);
      self.debugger.tlbStoreInvalid(vaddr);
      self.exception.tlbStoreInvalid();
    }
    return PhysAccess{false};
  }
  if(!entry.dirty[lo]) {
    if(!noExceptions) {
      self.addressException(vaddr);
      self.debugger.tlbModification(vaddr);
      self.exception.tlbModification();
    }
    return PhysAccess{false};
  }
  physicalAddress = entry.physicalAddress[lo] + (vaddr & entry.addressMaskLo);
  self.debugger.tlbStore(vaddr, physicalAddress);
  return PhysAccess{true, entry.cacheAlgorithm[lo] != 2, physicalAddress, vaddr};
}

auto CPU::TLB::store(u64 vaddr, bool noExceptions) -> PhysAccess {
  for(auto& entry : this->tlbCache.entry) {
    if(!entry.entry) continue;
    if(auto match = store(vaddr, *entry.entry)) {
      entry.frequency++;
      return *match;
    }
  }

  for(auto& entry : this->entry) {
    if(auto match = store(vaddr, entry, noExceptions)) {
      this->tlbCache.insert(entry);
      return *match;
    }
  }

  if(!noExceptions) {
    self.addressException(vaddr);
    self.debugger.tlbStoreMiss(vaddr);
    self.exception.tlbStoreMiss();
  }
  return {false};
}

auto CPU::TLB::Entry::synchronize() -> void {
  pageMask = pageMask & (0b101010101010 << 13);
  pageMask |= pageMask >> 1;
  globals = global[0] && global[1];
  addressMaskHi = ~(n40)(pageMask | 0x1fff);
  addressMaskLo = (pageMask | 0x1fff) >> 1;
  addressSelect = addressMaskLo + 1;
  physicalAddress[0] &= 0xffff'ffff;
  physicalAddress[1] &= 0xffff'ffff;
  virtualAddress &= addressMaskHi;
  global[0] = global[1] = globals;
}
