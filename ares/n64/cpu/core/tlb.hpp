//{
  //tlb.cpp
  auto tlbLookup(u64 address) -> u64;

  struct TLB {
     uint1 pageValid[2] = {};
     uint1 pageDirty[2] = {};
     uint3 cacheAlgorithm[2] = {};
    uint30 physicalAddress[2] = {};
     uint1 global = 0;
     uint8 addressSpaceID = 0;
    uint40 virtualAddress = 0;
    uint22 unused = 0;  //d40-d61 of virtual address (64-bit bus limit is 40-bits)
     uint2 region = 0;
    uint25 pageMask = 0;

    static constexpr uint Entries = 32;
  } tlb[TLB::Entries];
//};
