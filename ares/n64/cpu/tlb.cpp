//32-bit TLB

auto CPU::TLB::load32(u32 address) -> Match {
  for(auto& entry : this->entry) {
    if(!entry.globals || entry.addressSpaceID != self.scc.tlb.addressSpaceID) continue;
    if((address & entry.addressMaskHi) != (u32)entry.addressCompare) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) {
      exception32(address);
      self.debugger.tlbLoadInvalid(address);
      self.exception.tlbLoadInvalid();
      return {false};
    }
    physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
    self.debugger.tlbLoad(address, physicalAddress);
    return {true, entry.cacheAlgorithm[lo] != 2, physicalAddress};
  }
  exception32(address);
  self.debugger.tlbLoadMiss(address);
  self.exception.tlbLoadMiss();
  return {false};
}

auto CPU::TLB::store32(u32 address) -> Match {
  for(auto& entry : this->entry) {
    if(!entry.globals || entry.addressSpaceID != self.scc.tlb.addressSpaceID) continue;
    if((address & entry.addressMaskHi) != (u32)entry.addressCompare) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) {
      exception32(address);
      self.debugger.tlbStoreInvalid(address);
      self.exception.tlbStoreInvalid();
      return {false};
    }
    if(!entry.dirty[lo]) {
      exception32(address);
      self.debugger.tlbModification(address);
      self.exception.tlbModification();
      return {false};
    }
    physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
    self.debugger.tlbStore(address, physicalAddress);
    return {true, entry.cacheAlgorithm[lo] != 2, physicalAddress};
  }
  exception32(address);
  self.debugger.tlbStoreMiss(address);
  self.exception.tlbStoreMiss();
  return {false};
}

auto CPU::TLB::exception32(u32 address) -> void {
  self.scc.badVirtualAddress = address;
  self.scc.tlb.virtualAddress.bit(13,39) = address >> 13;
  self.scc.context.badVirtualAddress = address >> 13;
  self.scc.xcontext.badVirtualAddress = address >> 13;
  self.scc.xcontext.region = 0;
}

//64-bit TLB

auto CPU::TLB::load64(u64 address) -> Match {
  for(auto& entry : this->entry) {
    if(!entry.globals || entry.addressSpaceID != self.scc.tlb.addressSpaceID) continue;
    if((address & entry.addressMaskHi) != entry.addressCompare) continue;
    if(entry.region == 0b00 && self.context.mode != Context::Mode::User) continue;
    if(entry.region == 0b01 && self.context.mode != Context::Mode::Supervisor) continue;
    if(entry.region == 0b10) { debug(unusual, "[CPU::TLB::load64] region=2"); continue; }
    if(entry.region == 0b11 && self.context.mode != Context::Mode::Kernel) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) {
      exception64(address);
      self.debugger.tlbLoadInvalid(address);
      self.exception.tlbLoadInvalid();
      return {false};
    }
    physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
    self.debugger.tlbLoad(address, physicalAddress);
    return {true, entry.cacheAlgorithm[lo] != 2, physicalAddress};
  }
  exception64(address);
  self.debugger.tlbLoadMiss(address);
  self.exception.tlbLoadMiss();
  return {false};
}

auto CPU::TLB::store64(u64 address) -> Match {
  for(auto& entry : this->entry) {
    if(!entry.globals || entry.addressSpaceID != self.scc.tlb.addressSpaceID) continue;
    if((address & entry.addressMaskHi) != entry.addressCompare) continue;
    if(entry.region == 0b00 && self.context.mode != Context::Mode::User) continue;
    if(entry.region == 0b01 && self.context.mode != Context::Mode::Supervisor) continue;
    if(entry.region == 0b10) { debug(unusual, "[CPU::TLB::store64] region=2"); continue; }
    if(entry.region == 0b11 && self.context.mode != Context::Mode::Kernel) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) {
      exception64(address);
      self.debugger.tlbStoreInvalid(address);
      self.exception.tlbStoreInvalid();
      return {false};
    }
    if(!entry.dirty[lo]) {
      exception64(address);
      self.debugger.tlbModification(address);
      self.exception.tlbModification();
      return {false};
    }
    physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
    self.debugger.tlbStore(address, physicalAddress);
    return {true, entry.cacheAlgorithm[lo] != 2, physicalAddress};
  }
  exception64(address);
  self.debugger.tlbStoreMiss(address);
  self.exception.tlbStoreMiss();
  return {false};
}

auto CPU::TLB::exception64(u64 address) -> void {
  self.scc.badVirtualAddress = address;
  self.scc.tlb.virtualAddress.bit(13,39) = address >> 13;
  self.scc.context.badVirtualAddress = address >> 13;
  self.scc.xcontext.badVirtualAddress = address >> 13;
  self.scc.xcontext.region = address >> 62;
}

//synchronization

auto CPU::TLB::Entry::synchronize() -> void {
  globals = global[0] && global[1];
  addressMaskHi = ~(pageMask | 0x1fff);
  addressMaskLo = (pageMask | 0x1fff) >> 1;
  addressSelect = addressMaskLo + 1;
  addressCompare = virtualAddress & addressMaskHi;
}
