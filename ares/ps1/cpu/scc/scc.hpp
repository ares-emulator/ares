//struct CPU {
  struct SCC {
    struct Breakpoint {
      u32 lastPC;

      struct Address {
        // 3: Breakpoint Code Address
        n32 code;

        // 5: Breakpoint Data Address
        n32 data;
      } address;

      struct Mask {
        //11: Breakpoint Code Mask
        n32 code;

        // 9: Breakpoint Data Mask
        n32 data;
      } mask;

      // 7: Breakpoint Control
      struct Status {
        n1 any;
        n1 code;
        n1 data;
        n1 read;
        n1 write;
        n1 trace;
      } status;
      n2 redirection;
      n2 unknown;
      struct Test {
        n1 code;
        n1 data;
        n1 read;
        n1 write;
        n1 trace;
      } test;
      struct Enable {
        n1 master;
        n1 kernel;
        n1 user;
        n1 trap;
      } enable;
    } breakpoint;

    // 6: Target Address
    n32 targetAddress;

    // 8: Bad Virtual Address
    n32 badVirtualAddress;

    //12: Status
    struct Status {
      struct Frame {
        n1 interruptEnable;
        n1 userMode;
      } frame[3];
      n8 interruptMask;
      struct Cache {
        n1 isolate;
        n1 swap;
        n1 parityZero;
        n1 loadWasData;
        n1 parityError;
      } cache;
      n1 tlbShutdown;
      n1 vectorLocation;
      n1 reverseEndian;
      struct Enable {
        n1 coprocessor0 = 1;
        n1 coprocessor1;
        n1 coprocessor2 = 1;
        n1 coprocessor3;
      } enable;
    } status;

    //13: Cause
    struct Cause {
      n5 exceptionCode;
      n8 interruptPending;
      n2 coprocessorError;
      n1 branchTaken;
      n1 branchDelay;
    } cause;

    //14: Exception Program Counter
    n32 epc;

    //15: Product ID
    struct ProductID {
      static constexpr u8 implementation = 0x00;
      static constexpr u8 revision = 0x02;
    } productID;
  } scc;

  //scc/registers.cpp
  auto getControlRegisterSCC(u8) -> u32;
  auto setControlRegisterSCC(u8, u32) -> void;
  auto statusRegisterSCC() const -> u32;
  auto setStatusRegisterSCC(u32) -> void;
  auto effectiveStatusRegisterSCC() const -> u32;
  auto statusInterruptEnable() const -> bool;
  auto statusUserMode() const -> bool;
  auto statusCacheIsolated() const -> bool;
  auto statusVectorLocation() const -> bool;
  auto statusInterruptMask() const -> u8;
  auto statusCoprocessorEnabled(u32 coprocessor) const -> bool;

  //scc/breakpoints.cpp
  auto testCodeBreakpoint(u32 address) -> bool;
  template<u32 Mode, u32 Size> auto testDataBreakpoint(u32 address) -> bool;
//};
