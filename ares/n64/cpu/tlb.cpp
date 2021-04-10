auto CPU::TLB::load(u32 address) -> maybe<u32> {
  for(auto& entry : this->entry) {
    if(!entry.globals) continue;
    if((address & entry.addressMaskHi) != entry.addressCompare) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) {
      exception(address);
      self.exception.tlbLoadInvalid();
      return nothing;
    }
    physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
    return physicalAddress;
  }
  exception(address);
  self.exception.tlbLoadMiss();
  return nothing;
}

auto CPU::TLB::store(u32 address) -> maybe<u32> {
  for(auto& entry : this->entry) {
    if(!entry.globals) continue;
    if((address & entry.addressMaskHi) != entry.addressCompare) continue;
    bool lo = address & entry.addressSelect;
    if(!entry.valid[lo]) {
      exception(address);
      self.exception.tlbStoreInvalid();
      return nothing;
    }
    if(!entry.dirty[lo]) {
      exception(address);
      self.exception.tlbModification();
      return nothing;
    }
    physicalAddress = entry.physicalAddress[lo] + (address & entry.addressMaskLo);
    return physicalAddress;
  }
  exception(address);
  self.exception.tlbStoreMiss();
  return nothing;
}

auto CPU::TLB::exception(u32 address) -> void {
  self.scc.badVirtualAddress = address;
  self.scc.tlb.addressSpaceID = (s32)address >> 31;  //note: needs u64 address
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

//for debugging purposes
auto CPU::TLB::information() -> void {
  for(u32 index : range(32)) {
    auto& entry = this->entry[index];
    if(!entry.globals) continue;
    print(terminal::color::yellow("[TLB index: ", index, "]\n"));
    print("address mask:     0x", hex(entry.addressMaskHi,      8L), "\n");
    print("address compare:  0x", hex(entry.addressCompare,     8L), "\n");
    print("physical address: 0x", hex(entry.physicalAddress[0], 8L), " 0x", hex(entry.physicalAddress[1], 8L), "\n");
    print("virtual address:  0x", hex(entry.virtualAddress,     8L), "\n");
  } print("\n");
}
