//NEC VR4300

struct CPU : Thread {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto instruction() -> void;
    auto exception(string_view) -> void;
    auto interrupt(string_view) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification exception;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto synchronize() -> void;

  auto instruction() -> void;
  auto instructionEpilogue() -> bool;
  auto instructionDebug() -> void;

  auto power(bool reset) -> void;

  struct Pipeline {
    u32 address;
    u32 instruction;

    struct InstructionCache {
    } ic;

    struct RegisterFile {
    } rf;

    struct Execution {
    } ex;

    struct DataCache {
    } dc;

    struct WriteBack {
    } wb;
  } pipeline;

  struct Branch {
    enum : u32 { Step, Take, DelaySlot, Exception, Discard };

    auto inDelaySlot() const -> bool { return state == DelaySlot; }
    auto reset() -> void { state = Step; }
    auto take(u32 address) -> void { state = Take; pc = address; }
    auto delaySlot() -> void { state = DelaySlot; }
    auto exception() -> void { state = Exception; }
    auto discard() -> void { state = Discard; }

    u64 pc;
    u32 state;
  } branch;

  //context.cpp
  struct Context {
    CPU& self;
    Context(CPU& self) : self(self) {}

    enum Mode : uint { Kernel, Supervisor, User };
    enum Segment : uint { Invalid, Mapped, Cached, Uncached };
    uint mode;
    uint segment[8];  //512_MiB chunks

    auto setMode() -> void;
  } context{*this};

  //tlb.cpp: Translation Lookaside Buffer
  struct TLB {
    CPU& self;
    TLB(CPU& self) : self(self) {}
    static constexpr uint Entries = 32;

    //tlb.cpp
    auto load(u32 address) -> maybe<u32>;
    auto store(u32 address) -> maybe<u32>;
    auto exception(u32 address) -> void;

    struct Entry {
      //scc-tlb.cpp
      auto synchronize() -> void;

       uint1 global[2];
       uint1 valid[2];
       uint1 dirty[2];
       uint3 cacheAlgorithm[2];
      uint32 physicalAddress[2];
      uint32 pageMask;
      uint32 virtualAddress;
       uint8 addressSpaceID;
    //unimplemented:
      uint22 unused;
       uint2 region;
    //internal:
       uint1 globals;
      uint32 addressMaskHi;
      uint32 addressMaskLo;
      uint32 addressSelect;
      uint32 addressCompare;
    } entry[TLB::Entries];

    u32 physicalAddress;
  } tlb{*this};

  //memory.cpp
  auto readAddress (u32 address) -> maybe<u32>;
  auto writeAddress(u32 address) -> maybe<u32>;

  auto readByte    (u32 address) -> maybe<u32>;
  auto readHalf    (u32 address) -> maybe<u32>;
  auto readWord    (u32 address) -> maybe<u32>;
  auto readDouble  (u32 address) -> maybe<u64>;

  auto writeByte   (u32 address,  u8 data) -> bool;
  auto writeHalf   (u32 address, u16 data) -> bool;
  auto writeWord   (u32 address, u32 data) -> bool;
  auto writeDouble (u32 address, u64 data) -> bool;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //exception.cpp
  struct Exception {
    CPU& self;
    auto trigger(uint code, uint coprocessor = 0) -> void;
    auto interrupt() -> void;
    auto tlbModification() -> void;
    auto tlbLoad() -> void;
    auto tlbStore() -> void;
    auto addressLoad() -> void;
    auto addressStore() -> void;
    auto busInstruction() -> void;
    auto busData() -> void;
    auto systemCall() -> void;
    auto breakpoint() -> void;
    auto reservedInstruction() -> void;
    auto coprocessor0() -> void;
    auto coprocessor1() -> void;
    auto coprocessor2() -> void;
    auto coprocessor3() -> void;
    auto arithmeticOverflow() -> void;
    auto trap() -> void;
    auto floatingPoint() -> void;
    auto watchAddress() -> void;
  } exception{*this};

  enum Interrupt : uint {
    Software0 = 0,
    Software1 = 1,
    RCP       = 2,
    Cartridge = 3,
    Reset     = 4,
    ReadRDB   = 5,
    WriteRDB  = 6,
    Timer     = 7,
  };

  //ipu.cpp
  union r64 {
    struct {   int32_t order_msb2(s32h, s32); };
    struct {  uint32_t order_msb2(u32h, u32); };
    struct { float32_t order_msb2(f32h, f32); };
    struct {   int64_t s64; };
    struct {  uint64_t u64; };
    struct { float64_t f64; };
    auto s128() const ->  int128_t { return  (int128_t)s64; }
    auto u128() const -> uint128_t { return (uint128_t)u64; }
  };
  using cr64 = const r64;

  struct IPU {
    enum Register : uint {
      R0,                              //zero (read-only)
      AT,                              //assembler temporary
      V0, V1,                          //arithmetic values
      A0, A1, A2, A3,                  //subroutine parameters
      T0, T1, T2, T3, T4, T5, T6, T7,  //temporary registers
      S0, S1, S2, S3, S4, S5, S6, S7,  //saved registers
      T8, T9,                          //temporary registers
      K0, K1,                          //kernel registers
      GP,                              //global pointer
      SP,                              //stack pointer
      S8,                              //saved register
      RA,                              //return address
    };

    r64 r[32];
    r64 lo;
    r64 hi;
    u64 pc;  //program counter
  } ipu;

  auto instructionADD(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionADDI(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionADDIU(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionADDU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionAND(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionANDI(r64& rt, cr64& rs, u16 imm) -> void;
  auto instructionBEQ(cr64& rs, cr64& rt, s16 imm) -> void;
  auto instructionBEQL(cr64& rs, cr64& rt, s16 imm) -> void;
  auto instructionBGEZ(cr64& rs, s16 imm) -> void;
  auto instructionBGEZAL(cr64& rs, s16 imm) -> void;
  auto instructionBGEZALL(cr64& rs, s16 imm) -> void;
  auto instructionBGEZL(cr64& rs, s16 imm) -> void;
  auto instructionBGTZ(cr64& rs, s16 imm) -> void;
  auto instructionBGTZL(cr64& rs, s16 imm) -> void;
  auto instructionBLEZ(cr64& rs, s16 imm) -> void;
  auto instructionBLEZL(cr64& rs, s16 imm) -> void;
  auto instructionBLTZ(cr64& rs, s16 imm) -> void;
  auto instructionBLTZAL(cr64& rs, s16 imm) -> void;
  auto instructionBLTZALL(cr64& rs, s16 imm) -> void;
  auto instructionBLTZL(cr64& rs, s16 imm) -> void;
  auto instructionBNE(cr64& rs, cr64& rt, s16 imm) -> void;
  auto instructionBNEL(cr64& rs, cr64& rt, s16 imm) -> void;
  auto instructionBAL(bool take, s16 imm) -> void;
  auto instructionBALL(bool take, s16 imm) -> void;
  auto instructionBREAK() -> void;
  auto instructionCACHE(u8 cache, u8 operation) -> void;
  auto instructionDADD(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionDADDI(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionDADDIU(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionDADDU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionDDIV(cr64& rs, cr64& rt) -> void;
  auto instructionDDIVU(cr64& rs, cr64& rt) -> void;
  auto instructionDIV(cr64& rs, cr64& rt) -> void;
  auto instructionDIVU(cr64& rs, cr64& rt) -> void;
  auto instructionDMULT(cr64& rs, cr64& rt) -> void;
  auto instructionDMULTU(cr64& rs, cr64& rt) -> void;
  auto instructionDSLL(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionDSLLV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionDSRA(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionDSRAV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionDSRL(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionDSRLV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionDSUB(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionDSUBU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionJ(u32 imm) -> void;
  auto instructionJAL(u32 imm) -> void;
  auto instructionJALR(r64& rd, cr64& rs) -> void;
  auto instructionJR(cr64& rs) -> void;
  auto instructionLB(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLBU(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLD(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLDL(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLDR(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLH(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLHU(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLUI(r64& rt, u16 imm) -> void;
  auto instructionLL(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLLD(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLW(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLWL(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLWR(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionLWU(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionMFHI(r64& rd) -> void;
  auto instructionMFLO(r64& rd) -> void;
  auto instructionMTHI(cr64& rs) -> void;
  auto instructionMTLO(cr64& rs) -> void;
  auto instructionMULT(cr64& rs, cr64& rt) -> void;
  auto instructionMULTU(cr64& rs, cr64& rt) -> void;
  auto instructionNOR(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionOR(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionORI(r64& rt, cr64& rs, u16 imm) -> void;
  auto instructionSB(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSC(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSD(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSCD(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSDL(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSDR(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSH(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSLL(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionSLLV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionSLT(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSLTI(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSLTIU(r64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSLTU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSRA(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionSRAV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionSRL(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionSRLV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionSUB(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSUBU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSW(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSWL(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSWR(cr64& rt, cr64& rs, s16 imm) -> void;
  auto instructionSYNC() -> void;
  auto instructionSYSCALL() -> void;
  auto instructionTEQ(cr64& rs, cr64& rt) -> void;
  auto instructionTEQI(cr64& rs, s16 imm) -> void;
  auto instructionTGE(cr64& rs, cr64& rt) -> void;
  auto instructionTGEI(cr64& rs, s16 imm) -> void;
  auto instructionTGEIU(cr64& rs, s16 imm) -> void;
  auto instructionTGEU(cr64& rs, cr64& rt) -> void;
  auto instructionTLT(cr64& rs, cr64& rt) -> void;
  auto instructionTLTI(cr64& rs, s16 imm) -> void;
  auto instructionTLTIU(cr64& rs, s16 imm) -> void;
  auto instructionTLTU(cr64& rs, cr64& rt) -> void;
  auto instructionTNE(cr64& rs, cr64& rt) -> void;
  auto instructionTNEI(cr64& rs, s16 imm) -> void;
  auto instructionXOR(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionXORI(r64& rt, cr64& rs, u16 imm) -> void;

  //scc.cpp: System Control Coprocessor
  struct SCC {
    //0
    struct Index {
      uint6 tlbEntry;
      uint1 probeFailure;
    } index;

    //1
    struct Random {
      uint5 index;
      uint1 unused;
    } random;

    //2: EntryLo0
    //3: EntryLo1
    //5: PageMask
    //10: EntryHi
    TLB::Entry tlb;

    //4
    struct Context {
      uint19 badVirtualAddress;
      uint41 pageTableEntryBase;
    } context;

    //6
    struct Wired {
      uint5 index;
      uint1 unused;
    } wired;

    //8
    u64 badVirtualAddress;

    //9
    u32 count;

    //11
    u32 compare;

    //12
    struct Status {
      uint1 interruptEnable;
      uint1 exceptionLevel;
      uint1 errorLevel;
      uint2 privilegeMode;
      uint1 userExtendedAddressing;
      uint1 supervisorExtendedAddressing;
      uint1 kernelExtendedAddressing;
      uint8 interruptMask;
      uint1 de;  //unused
      uint1 ce;  //unused
      uint1 condition;
      uint1 softReset;
      uint1 tlbShutdown;
      uint1 vectorLocation;
      uint1 instructionTracing;
      uint1 reverseEndian;
      uint1 floatingPointMode;
      uint1 lowPowerMode;
      struct Enable {
        uint1 coprocessor0;
        uint1 coprocessor1;
        uint1 coprocessor2;
        uint1 coprocessor3;
      } enable;
    } status;

    //13
    struct Cause {
      uint5 exceptionCode;
      uint8 interruptPending;
      uint2 coprocessorError;
      uint1 branchDelay;
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
      uint2 coherencyAlgorithmKSEG0;
      uint2 cu;  //reserved
      uint1 bigEndian;
      uint2 sysadWritebackPattern;
      uint2 systemClockRatio;
    } configuration;

    //17: Load Linked Address
    u64 ll;
    uint1 llbit;

    //18
    struct WatchLo {
       uint1 trapOnWrite;
       uint1 trapOnRead;
      uint32 physicalAddress;
    } watchLo;

    //19
    struct WatchHi {
      uint4 physicalAddressExtended;  //unused; for R4000 compatibility only
    } watchHi;

    //20
    struct XContext {
      uint27 badVirtualAddress;
       uint2 region;
      uint31 pageTableEntryBase;
    } xcontext;

    //26
    struct ParityError {
      uint8 diagnostic;  //unused; for R4000 compatibility only
    } parityError;

    //28
    struct TagLo {
       uint2 primaryCacheState;
      uint32 physicalAddress;
    } tagLo;

    //31: Error Exception Program Counter
    u64 epcError;
  } scc;

  auto getControlRegister(uint5) -> u64;
  auto setControlRegister(uint5, uint64) -> void;

  auto instructionDMFC0(r64& rt, u8 rd) -> void;
  auto instructionDMTC0(cr64& rt, u8 rd) -> void;
  auto instructionERET() -> void;
  auto instructionMFC0(r64& rt, u8 rd) -> void;
  auto instructionMTC0(cr64& rt, u8 rd) -> void;
  auto instructionTLBP() -> void;
  auto instructionTLBR() -> void;
  auto instructionTLBWI() -> void;
  auto instructionTLBWR() -> void;

  //fpu.cpp: Floating-Point Unit
  struct FPU {
    auto setFloatingPointMode(bool) -> void;

    r64 r[32];

    struct Coprocessor {
      static constexpr u8 revision = 0x00;
      static constexpr u8 implementation = 0x0b;
    } coprocessor;

    struct ControlStatus {
      uint2 roundMode = 0;
      struct Flag {
        uint1 inexact = 0;
        uint1 underflow = 0;
        uint1 overflow = 0;
        uint1 divisionByZero = 0;
        uint1 invalidOperation = 0;
      } flag;
      struct Enable {
        uint1 inexact = 0;
        uint1 underflow = 0;
        uint1 overflow = 0;
        uint1 divisionByZero = 0;
        uint1 invalidOperation = 0;
      } enable;
      struct Cause {
        uint1 inexact = 0;
        uint1 underflow = 0;
        uint1 overflow = 0;
        uint1 divisionByZero = 0;
        uint1 invalidOperation = 0;
        uint1 unimplementedOperation = 0;
      } cause;
      uint1 compare = 0;
      uint1 flushed = 0;
    } csr;
  } fpu;

  template<typename T> auto fgr(uint) -> T&;
  auto getControlRegisterFPU(uint5) -> u32;
  auto setControlRegisterFPU(uint5, uint32) -> void;

  auto instructionBC1(bool value, bool likely, s16 imm) -> void;
  auto instructionCFC1(r64& rt, u8 rd) -> void;
  auto instructionCTC1(cr64& rt, u8 rd) -> void;
  auto instructionDMFC1(r64& rt, u8 fs) -> void;
  auto instructionDMTC1(cr64& rt, u8 fs) -> void;
  auto instructionFABS_S(u8 fd, u8 fs) -> void;
  auto instructionFABS_D(u8 fd, u8 fs) -> void;
  auto instructionFADD_S(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFADD_D(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFCEIL_L_S(u8 fd, u8 fs) -> void;
  auto instructionFCEIL_L_D(u8 fd, u8 fs) -> void;
  auto instructionFCEIL_W_S(u8 fd, u8 fs) -> void;
  auto instructionFCEIL_W_D(u8 fd, u8 fs) -> void;
  auto instructionFC_EQ_S(u8 fs, u8 ft) -> void;
  auto instructionFC_EQ_D(u8 fs, u8 ft) -> void;
  auto instructionFC_F_S(u8 fs, u8 ft) -> void;
  auto instructionFC_F_D(u8 fs, u8 ft) -> void;
  auto instructionFC_LE_S(u8 fs, u8 ft) -> void;
  auto instructionFC_LE_D(u8 fs, u8 ft) -> void;
  auto instructionFC_LT_S(u8 fs, u8 ft) -> void;
  auto instructionFC_LT_D(u8 fs, u8 ft) -> void;
  auto instructionFC_NGE_S(u8 fs, u8 ft) -> void;
  auto instructionFC_NGE_D(u8 fs, u8 ft) -> void;
  auto instructionFC_NGL_S(u8 fs, u8 ft) -> void;
  auto instructionFC_NGL_D(u8 fs, u8 ft) -> void;
  auto instructionFC_NGLE_S(u8 fs, u8 ft) -> void;
  auto instructionFC_NGLE_D(u8 fs, u8 ft) -> void;
  auto instructionFC_NGT_S(u8 fs, u8 ft) -> void;
  auto instructionFC_NGT_D(u8 fs, u8 ft) -> void;
  auto instructionFC_OLE_S(u8 fs, u8 ft) -> void;
  auto instructionFC_OLE_D(u8 fs, u8 ft) -> void;
  auto instructionFC_OLT_S(u8 fs, u8 ft) -> void;
  auto instructionFC_OLT_D(u8 fs, u8 ft) -> void;
  auto instructionFC_SEQ_S(u8 fs, u8 ft) -> void;
  auto instructionFC_SEQ_D(u8 fs, u8 ft) -> void;
  auto instructionFC_SF_S(u8 fs, u8 ft) -> void;
  auto instructionFC_SF_D(u8 fs, u8 ft) -> void;
  auto instructionFC_UEQ_S(u8 fs, u8 ft) -> void;
  auto instructionFC_UEQ_D(u8 fs, u8 ft) -> void;
  auto instructionFC_ULE_S(u8 fs, u8 ft) -> void;
  auto instructionFC_ULE_D(u8 fs, u8 ft) -> void;
  auto instructionFC_ULT_S(u8 fs, u8 ft) -> void;
  auto instructionFC_ULT_D(u8 fs, u8 ft) -> void;
  auto instructionFC_UN_S(u8 fs, u8 ft) -> void;
  auto instructionFC_UN_D(u8 fs, u8 ft) -> void;
  auto instructionFCVT_S_D(u8 fd, u8 fs) -> void;
  auto instructionFCVT_S_W(u8 fd, u8 fs) -> void;
  auto instructionFCVT_S_L(u8 fd, u8 fs) -> void;
  auto instructionFCVT_D_S(u8 fd, u8 fs) -> void;
  auto instructionFCVT_D_W(u8 fd, u8 fs) -> void;
  auto instructionFCVT_D_L(u8 fd, u8 fs) -> void;
  auto instructionFCVT_L_S(u8 fd, u8 fs) -> void;
  auto instructionFCVT_L_D(u8 fd, u8 fs) -> void;
  auto instructionFCVT_W_S(u8 fd, u8 fs) -> void;
  auto instructionFCVT_W_D(u8 fd, u8 fs) -> void;
  auto instructionFDIV_S(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFDIV_D(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFFLOOR_L_S(u8 fd, u8 fs) -> void;
  auto instructionFFLOOR_L_D(u8 fd, u8 fs) -> void;
  auto instructionFFLOOR_W_S(u8 fd, u8 fs) -> void;
  auto instructionFFLOOR_W_D(u8 fd, u8 fs) -> void;
  auto instructionFMOV_S(u8 fd, u8 fs) -> void;
  auto instructionFMOV_D(u8 fd, u8 fs) -> void;
  auto instructionFMUL_S(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFMUL_D(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFNEG_S(u8 fd, u8 fs) -> void;
  auto instructionFNEG_D(u8 fd, u8 fs) -> void;
  auto instructionFROUND_L_S(u8 fd, u8 fs) -> void;
  auto instructionFROUND_L_D(u8 fd, u8 fs) -> void;
  auto instructionFROUND_W_S(u8 fd, u8 fs) -> void;
  auto instructionFROUND_W_D(u8 fd, u8 fs) -> void;
  auto instructionFSQRT_S(u8 fd, u8 fs) -> void;
  auto instructionFSQRT_D(u8 fd, u8 fs) -> void;
  auto instructionFSUB_S(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFSUB_D(u8 fd, u8 fs, u8 ft) -> void;
  auto instructionFTRUNC_L_S(u8 fd, u8 fs) -> void;
  auto instructionFTRUNC_L_D(u8 fd, u8 fs) -> void;
  auto instructionFTRUNC_W_S(u8 fd, u8 fs) -> void;
  auto instructionFTRUNC_W_D(u8 fd, u8 fs) -> void;
  auto instructionLDC1(u8 ft, cr64& rs, s16 imm) -> void;
  auto instructionLWC1(u8 ft, cr64& rs, s16 imm) -> void;
  auto instructionMFC1(r64& rt, u8 fs) -> void;
  auto instructionMTC1(cr64& rt, u8 fs) -> void;
  auto instructionSDC1(u8 ft, cr64& rs, s16 imm) -> void;
  auto instructionSWC1(u8 ft, cr64& rs, s16 imm) -> void;

  //decoder.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderSCC() -> void;
  auto decoderFPU() -> void;

  auto instructionCOP2() -> void;
  auto instructionCOP3() -> void;
  auto instructionINVALID() -> void;

  //recompiler.cpp
  struct Recompiler : recompiler::amd64 {
    CPU& self;
    Recompiler(CPU& self) : self(self) {}

    struct Block {
      auto execute() -> void {
        ((void (*)())code)();
      }

      u8* code;
    };

    struct Pool {
      Block* blocks[1 << 6];
    };

    auto reset() -> void {
      for(u32 index : range(1 << 21)) pools[index] = nullptr;
    }

    auto invalidate(u32 address) -> void {
      pools[address >> 8 & 0x1fffff] = nullptr;
    }

    auto pool(u32 address) -> Pool*;
    auto block(u32 address) -> Block*;

    auto emit(u32 address) -> Block*;
    auto emitEXECUTE(u32 instruction) -> bool;
    auto emitSPECIAL(u32 instruction) -> bool;
    auto emitREGIMM(u32 instruction) -> bool;
    auto emitSCC(u32 instruction) -> bool;
    auto emitFPU(u32 instruction) -> bool;
    auto emitCheckFPU() -> bool;

    bump_allocator allocator;
    Pool* pools[1 << 21];  //2_MiB * sizeof(void*) == 16_MiB
  } recompiler{*this};

  struct Disassembler {
    CPU& self;
    Disassembler(CPU& self) : self(self) {}

    //disassembler.cpp
    auto disassemble(u32 address, u32 instruction) -> string;
    template<typename... P> auto hint(P&&... p) const -> string;

    bool showColors = true;
    bool showValues = true;

  private:
    auto EXECUTE() -> vector<string>;
    auto SPECIAL() -> vector<string>;
    auto REGIMM() -> vector<string>;
    auto SCC() -> vector<string>;
    auto FPU() -> vector<string>;
    auto immediate(s64 value, uint bits = 0) const -> string;
    auto ipuRegisterName(uint index) const -> string;
    auto ipuRegisterValue(uint index) const -> string;
    auto ipuRegisterIndex(uint index, s16 offset) const -> string;
    auto sccRegisterName(uint index) const -> string;
    auto sccRegisterValue(uint index) const -> string;
    auto fpuRegisterName(uint index) const -> string;
    auto fpuRegisterValue(uint index) const -> string;
    auto ccrRegisterName(uint index) const -> string;
    auto ccrRegisterValue(uint index) const -> string;

    u32 address;
    u32 instruction;
  } disassembler{*this};

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);
};

extern CPU cpu;
