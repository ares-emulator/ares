//{
  struct TLB {
    CPU& self;
    TLB(CPU& self) : self(self) {}
    static constexpr uint Entries = 32;

    //tlb.cpp
    auto load(u32 address) -> maybe<u32>;
    auto store(u32 address) -> maybe<u32>;
    auto exception(u32 address) -> void;
    auto rebuild() -> void;

    struct Entry {
      bool valid = 0;
      bool dirty = 0;
      bool cached = 0;
      bool global = 0;
      u32 physicalAddress = 0;
      u32 virtualAddress = 0;
      u32 mask = 0;
    //unimplemented:
       u8 addressSpaceID = 0;
       u8 region = 0;
    } entry[TLB::Entries * 2];  //even + odd entries

    enum : u8 { Invalid = 0xff };
    u8 pageTable[1_MiB];  //d12-d31 lookup to TLB entry
  } tlb{*this};
//};
