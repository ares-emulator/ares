//{
  //scc-registers.cpp
  auto getControlRegister(u8) -> u32;
  auto setControlRegister(u8, u32) -> void;

  //System Control Coprocessor
  struct SCC {
    // 3: Breakpoint Execute Address
    u32 breakpointExecuteAddress = 0;

    // 5: Breakpoint Data Address
    u32 breakpointDataAddress = 0;

    // 7: Breakpoint Control

    // 8: Bad Virtual Address
    u32 badVirtualAddress = 0;

    // 9: Breakpoint Data Mask
    u32 breakpointDataMask = 0;

    //11: Breakpoint Execute Mask
    u32 breakpointExecuteMask = 0;

    //12: Status
    struct Status {
      struct Frame {
        uint1 interruptEnable = 0;
        uint1 userMode = 0;
      } frame[3];
      uint8 interruptMask = 0;
      struct Cache {
        uint1 isolate = 0;
        uint1 swap = 0;
        uint1 parityZero = 0;
        uint1 loadWasData = 0;
        uint1 parityError = 0;
      } cache;
      uint1 tlbShutdown = 0;
      uint1 vectorLocation = 0;
      uint1 reverseEndian = 0;
      struct Enable {
        uint1 coprocessor0 = 1;
        uint1 coprocessor1 = 0;
        uint1 coprocessor2 = 1;
        uint1 coprocessor3 = 0;
      } enable;
    } status;

    //13: Cause
    struct Cause {
      uint5 exceptionCode = 0;
      uint8 interruptPending = 0x00;
      uint2 coprocessorError = 0;
      uint1 branchDelay = 0;
    } cause;

    //14: Exception Program Counter
    u32 epc = 0;

    //15: Product ID
    struct ProductID {
      u8 implementation = 0x00;
      u8 revision = 0x02;
    } productID;
  } scc;

  //scc-instructions.cpp
  auto instructionMFC0(u32& rt, u8 rd) -> void;
  auto instructionMTC0(cu32& rt, u8 rd) -> void;
  auto instructionRFE() -> void;
//};
