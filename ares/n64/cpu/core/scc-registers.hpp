//{
  //scc-registers.cpp
  auto getControlRegister(uint5) -> u64;
  auto setControlRegister(uint5, uint64) -> void;

  //System Control Coprocessor
  struct SCC {
    //0
    struct Index {
      uint6 tlbEntry = 0;
      uint1 probeFailure = 0;
    } index;

    //1
    struct Random {
      uint5 index = 31;
      uint1 unused = 0;
    } random;

    //2: EntryLo0
    //3: EntryLo1
    struct EntryLo {
       uint1 global = 0;
       uint1 pageValid = 0;
       uint1 pageDirty = 0;
       uint3 cacheAlgorithm = 0;
      uint24 physicalAddress = 0;
    } entryLo[2];

    //4
    struct Context {
      uint19 badVirtualAddress = 0;
      uint41 pageTableEntryBase = 0;
    } context;

    //5: PageMask
    uint25 pageMask = 0;

    //6
    struct Wired {
      uint5 index = 0;
      uint1 unused = 0;
    } wired;

    //8
    u64 badVirtualAddress = 0;

    //9
    u32 count = 0;

    //10
    struct EntryHi {
       uint8 addressSpaceID = 0;
      uint27 virtualAddress = 0;
      uint22 unused = 0;
       uint2 region = 0;
    } entryHi;

    //11
    u32 compare = 0;

    //12
    struct Status {
      uint1 interruptEnable = 0;
      uint1 exceptionLevel = 0;
      uint1 errorLevel = 1;
      uint2 privilegeMode = 0;
      uint1 userExtendedAddressing = 0;
      uint1 supervisorExtendedAddressing = 0;
      uint1 kernelExtendedAddressing = 0;
      uint8 interruptMask = 0xff;
      uint1 de = 0;  //unused
      uint1 ce = 0;  //unused
      uint1 condition = 0;
      uint1 softReset = 0;
      uint1 tlbShutdown = 0;
      uint1 vectorLocation = 1;
      uint1 instructionTracing = 0;
      uint1 reverseEndian = 0;
      uint1 floatingPointMode = 1;
      uint1 lowPowerMode = 0;
      struct Enable {
        uint1 coprocessor0 = 1;
        uint1 coprocessor1 = 1;
        uint1 coprocessor2 = 0;
        uint1 coprocessor3 = 0;
      } enable;
    } status;

    //13
    struct Cause {
      uint5 exceptionCode = 0;
      uint8 interruptPending = 0x00;
      uint2 coprocessorError = 0;
      uint1 branchDelay = 0;
    } cause;

    //14: Exception Program Counter
    u64 epc;

    //15: Coprocessor Revision Identifier
    struct Coprocessor {
      static constexpr u8 revision = 0x00;
      static constexpr u8 implementation = 0x0b;
    } coprocessor;

    //16
    struct Configuration {
      uint2 coherencyAlgorithmKSEG0 = 0;
      uint2 cu = 0;  //reserved
      uint1 bigEndian = 1;
      uint2 sysadWritebackPattern = 0;
      uint2 systemClockRatio = 6;
    } configuration;

    //17: Load Linked Address
    u64 ll;
    uint1 llbit;

    //18
    struct WatchLo {
       uint1 trapOnWrite = 0;
       uint1 trapOnRead = 0;
      uint32 physicalAddress = 0;
    } watchLo;

    //19
    struct WatchHi {
      uint4 physicalAddressExtended = 0;  //unused; for R4000 compatibility only
    } watchHi;

    //20
    struct XContext {
      uint27 badVirtualAddress = 0;
       uint2 region = 0;
      uint31 pageTableEntryBase = 0;
    } xcontext;

    //26
    struct ParityError {
      uint8 diagnostic = 0;  //unused; for R4000 compatibility only
    } parityError;

    //28
    struct TagLo {
       uint2 primaryCacheState = 0;
      uint32 physicalAddress = 0;
    } tagLo;

    //31: Error Exception Program Counter
    u64 epcError;

    u32 cr[32];
    bool cf = 0;
  } scc;
//};
