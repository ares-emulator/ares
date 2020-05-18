auto CPU::TLB::load(u32 address) -> maybe<u32> {
  i8 index = pageTable[address >> 12];
  if(index == Invalid) return self.exception.tlbLoad(), nothing;
  if(!entry[index].valid) return self.exception.tlbLoad(), nothing;
  return physicalAddress = entry[index].physicalAddress | address & 0xfff;
}

auto CPU::TLB::store(u32 address) -> maybe<u32> {
  i8 index = pageTable[address >> 12];
  if(index == Invalid) return self.exception.tlbStore(), nothing;
  if(!entry[index].valid) return self.exception.tlbStore(), nothing;
  if(!entry[index].dirty) return self.exception.tlbModification(), nothing;
  return physicalAddress = entry[index].physicalAddress | address & 0xfff;
}

auto CPU::TLB::exception(u32 address) -> void {
  self.scc.badVirtualAddress = address;
  self.scc.entryHi.addressSpaceID = address >> 32;  //note: needs u64 address
  self.scc.entryHi.virtualAddress = address >> 13;
  self.scc.context.badVirtualAddress = address >> 13;
  self.scc.xcontext.badVirtualAddress = address >> 13;
}

auto CPU::TLB::rebuild() -> void {
  for(u32 index : range(1_MiB)) {
    pageTable[index] = Invalid;
  }

  for(uint index : range(Entries)) {
    u32 virtualAddress = entry[index].virtualAddress;
    u32 mask = entry[index].mask >> 12;
    for(uint offset : range(mask)) {
      if(pageTable[virtualAddress + offset] != Invalid) {
        self.scc.status.tlbShutdown = 1;
      }
      pageTable[virtualAddress + offset] = index << 1 | offset & 1;
    }
  }
}
