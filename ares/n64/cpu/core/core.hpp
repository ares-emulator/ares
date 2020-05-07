//{
  //core.cpp
  auto powerR4300() -> void;

  //memory.cpp
  auto readByte(u32 address) -> maybe<u32>;
  auto readHalf(u32 address) -> maybe<u32>;
  auto readWord(u32 address) -> maybe<u32>;
  auto readDouble(u32 address) -> maybe<u64>;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeDouble(u32 address, u64 data) -> void;

  union r64 {
    struct {   int32_t order_msb2(i32h, i32); };
    struct {  uint32_t order_msb2(u32h, u32); };
    struct { float32_t order_msb2(f32h, f32); };
    struct {   int64_t i64; };
    struct {  uint64_t u64; };
    struct { float64_t f64; };
    auto i128() const ->  int128_t { return  (int128_t)i64; }
    auto u128() const -> uint128_t { return (uint128_t)u64; }
  };

  //instruction.cpp
  auto raiseException(uint, uint = 0, uint = 0) -> void;
  auto instruction() -> void;
  auto instructionDEBUG() -> void;
  auto instructionEXECUTE() -> void;
  auto instructionSPECIAL() -> void;
  auto instructionREGIMM() -> void;
  auto instructionCOP0() -> void;
  auto instructionCOP1() -> void;

  //instructions-cpu.cpp
  auto instructionADD() -> void;
  auto instructionADDI() -> void;
  auto instructionADDIU() -> void;
  auto instructionADDU() -> void;
  auto instructionAND() -> void;
  auto instructionANDI() -> void;
  auto instructionB(bool) -> void;
  auto instructionBAL(bool) -> void;
  auto instructionBALL(bool) -> void;
  auto instructionBL(bool) -> void;
  auto instructionBREAK() -> void;
  auto instructionCACHE() -> void;
  auto instructionDADD() -> void;
  auto instructionDADDI() -> void;
  auto instructionDADDIU() -> void;
  auto instructionDADDU() -> void;
  auto instructionDDIV() -> void;
  auto instructionDDIVU() -> void;
  auto instructionDIV() -> void;
  auto instructionDIVU() -> void;
  auto instructionDMULT() -> void;
  auto instructionDMULTU() -> void;
  auto instructionDSLL() -> void;
  auto instructionDSLL32() -> void;
  auto instructionDSLLV() -> void;
  auto instructionDSRA() -> void;
  auto instructionDSRA32() -> void;
  auto instructionDSRAV() -> void;
  auto instructionDSRL() -> void;
  auto instructionDSRL32() -> void;
  auto instructionDSRLV() -> void;
  auto instructionDSUB() -> void;
  auto instructionDSUBU() -> void;
  auto instructionJ() -> void;
  auto instructionJAL() -> void;
  auto instructionJALR() -> void;
  auto instructionJR() -> void;
  auto instructionLB() -> void;
  auto instructionLBU() -> void;
  auto instructionLD() -> void;
  auto instructionLDL() -> void;
  auto instructionLDR() -> void;
  auto instructionLH() -> void;
  auto instructionLHU() -> void;
  auto instructionLUI() -> void;
  auto instructionLL() -> void;
  auto instructionLLD() -> void;
  auto instructionLW() -> void;
  auto instructionLWL() -> void;
  auto instructionLWR() -> void;
  auto instructionLWU() -> void;
  auto instructionMFHI() -> void;
  auto instructionMFLO() -> void;
  auto instructionMTHI() -> void;
  auto instructionMTLO() -> void;
  auto instructionMULT() -> void;
  auto instructionMULTU() -> void;
  auto instructionNOR() -> void;
  auto instructionOR() -> void;
  auto instructionORI() -> void;
  auto instructionSB() -> void;
  auto instructionSC() -> void;
  auto instructionSD() -> void;
  auto instructionSCD() -> void;
  auto instructionSDL() -> void;
  auto instructionSDR() -> void;
  auto instructionSH() -> void;
  auto instructionSLL() -> void;
  auto instructionSLLV() -> void;
  auto instructionSLT() -> void;
  auto instructionSLTI() -> void;
  auto instructionSLTIU() -> void;
  auto instructionSLTU() -> void;
  auto instructionSRA() -> void;
  auto instructionSRAV() -> void;
  auto instructionSRL() -> void;
  auto instructionSRLV() -> void;
  auto instructionSUB() -> void;
  auto instructionSUBU() -> void;
  auto instructionSW() -> void;
  auto instructionSWL() -> void;
  auto instructionSWR() -> void;
  auto instructionSYSCALL() -> void;
  auto instructionTEQ() -> void;
  auto instructionTEQI() -> void;
  auto instructionTGE() -> void;
  auto instructionTGEI() -> void;
  auto instructionTGEIU() -> void;
  auto instructionTGEU() -> void;
  auto instructionTLT() -> void;
  auto instructionTLTI() -> void;
  auto instructionTLTIU() -> void;
  auto instructionTLTU() -> void;
  auto instructionTNE() -> void;
  auto instructionTNEI() -> void;
  auto instructionXOR() -> void;
  auto instructionXORI() -> void;

  //instructions-scc.cpp
  auto getControlRegister(uint5) -> u64;
  auto setControlRegister(uint5, uint64) -> void;

  auto instructionBC0() -> void;
  auto instructionCFC0() -> void;
  auto instructionCTC0() -> void;
  auto instructionDMFC0() -> void;
  auto instructionDMTC0() -> void;
  auto instructionMFC0() -> void;
  auto instructionMTC0() -> void;
  auto instructionTLBP() -> void;
  auto instructionTLBR() -> void;
  auto instructionTLBWI() -> void;
  auto instructionTLBWR() -> void;

  //instructions-fpu.cpp
  auto instructionBC1() -> void;
  auto instructionCFC1() -> void;
  auto instructionCTC1() -> void;
  auto instructionDMFC1() -> void;
  auto instructionDMTC1() -> void;
  auto instructionFABS() -> void;
  auto instructionFADD() -> void;
  auto instructionFCEILL() -> void;
  auto instructionFCEILW() -> void;
  auto instructionFCEQ() -> void;
  auto instructionFCF() -> void;
  auto instructionFCOLE() -> void;
  auto instructionFCOLT() -> void;
  auto instructionFCUEQ() -> void;
  auto instructionFCULE() -> void;
  auto instructionFCULT() -> void;
  auto instructionFCUN() -> void;
  auto instructionFCVTS() -> void;
  auto instructionFCVTD() -> void;
  auto instructionFCVTL() -> void;
  auto instructionFCVTW() -> void;
  auto instructionFDIV() -> void;
  auto instructionFFLOORL() -> void;
  auto instructionFFLOORW() -> void;
  auto instructionFMOV() -> void;
  auto instructionFMUL() -> void;
  auto instructionFNEG() -> void;
  auto instructionFROUNDL() -> void;
  auto instructionFROUNDW() -> void;
  auto instructionFSQRT() -> void;
  auto instructionFSUB() -> void;
  auto instructionFTRUNCL() -> void;
  auto instructionFTRUNCW() -> void;
  auto instructionLDC1() -> void;
  auto instructionLWC1() -> void;
  auto instructionMFC1() -> void;
  auto instructionMTC1() -> void;
  auto instructionSDC1() -> void;
  auto instructionSWC1() -> void;

  struct Exception {
    CPU& self;
    auto interruptRCP() { self.raiseException(0, 0, 1 << 2); }
    auto interruptDD() { self.raiseException(0, 0, 1 << 3); }
    auto interruptCompare() { self.raiseException(0, 0, 1 << 7); }
    auto tlbModification() { self.raiseException(1); }
    auto tlbLoad() { self.raiseException(2); }
    auto tlbStore() { self.raiseException(3); }
    auto addressLoad() { self.raiseException(4); }
    auto addressStore() { self.raiseException(5); }
    auto busInstruction() { self.raiseException(6); }
    auto busData() { self.raiseException(7); }
    auto systemCall() { self.raiseException(8); }
    auto breakpoint() { self.raiseException(9); }
    auto reservedInstruction() { self.raiseException(10); }
    auto coprocessor0() { self.raiseException(11, 0); }
    auto coprocessor1() { self.raiseException(11, 1); }
    auto coprocessor2() { self.raiseException(11, 2); }
    auto coprocessor3() { self.raiseException(11, 3); }
    auto arithmeticOverflow() { self.raiseException(12); }
    auto trap() { self.raiseException(13); }
    auto floatingPoint() { self.raiseException(15); }
    auto watchAddress() { self.raiseException(23); }
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

  //serialization.cpp
  auto serializeR4300(serializer&) -> void;

  struct Pipeline {
    u32 address;
    u32 instruction;
  } pipeline;

  struct Core {
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
    u64 pc;   //program counter

    //internal
    maybe<u64> ip;
  } core;

  //System Control Coprocessor
  struct SCC {
    //12: Status Register
    struct Status {
      uint1 interruptEnable = 1;
      uint1 exceptionLevel = 0;
      uint1 errorLevel = 0;
      uint2 privilegeMode = 0;
      uint1 userMode = 0;
      uint1 supervisorMode = 0;
      uint1 kernelMode = 0;
      uint8 interruptMask = 0xff;
      uint1 de = 0;  //unused
      uint1 ce = 0;  //unused
      uint1 condition = 0;
      uint1 softReset = 0;
      uint1 tlbShutdown = 0;
      uint1 vectorLocation = 0;
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

    //13: Cause Register
    struct Cause {
      uint5 exceptionCode = 0;
      uint8 interruptPending = 0x00;
      uint2 coprocessorError = 0;
      uint1 branchDelay = 0;
    } cause;

    //14: Exception Program Counter
    r64 epc;

    //17: Load Linked Address
    r64 ll;
    uint1 llbit;

    u32 cr[32];
    bool cf = 0;
  } scc;

  struct FPU {
    r64 r[32];
    u32 cr[32];
    bool cf[8];
  } fpu;

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);

  #include "disassembler.hpp"
//};
