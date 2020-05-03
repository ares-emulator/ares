//{
  //core.cpp
  auto powerR4300() -> void;

  //memory.cpp
  auto readByte(u32 address) -> maybe<u32>;
  auto readHalf(u32 address) -> maybe<u32>;
  auto readWord(u32 address) -> maybe<u32>;
  auto readWord(u32 address, u32 mask) -> maybe<u32>;
  auto readDouble(u32 address) -> maybe<u64>;
  auto readDouble(u32 address, u32 mask) -> maybe<u64>;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 mask, u32 data) -> void;
  auto writeDouble(u32 address, u64 data) -> void;
  auto writeDouble(u32 address, u32 mask, u64 data) -> void;

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
  auto exception(uint type) -> void;
  auto instruction() -> void;
  auto instructionDEBUG() -> void;
  auto instructionEXECUTE() -> void;
  auto instructionSPECIAL() -> void;
  auto instructionREGIMM() -> void;
  auto instructionCOP0() -> void;
  auto instructionCOP1() -> void;
  auto instructionCOP2() -> void;
  auto instructionCOP3() -> void;

  //instructions-core.cpp
  auto instructionADD() -> void;
  auto instructionADDI() -> void;
  auto instructionADDIU() -> void;
  auto instructionADDU() -> void;
  auto instructionAND() -> void;
  auto instructionANDI() -> void;
  auto instructionBEQ() -> void;
  auto instructionBEQL() -> void;
  auto instructionBGEZ() -> void;
  auto instructionBGEZAL() -> void;
  auto instructionBGEZALL() -> void;
  auto instructionBGEZL() -> void;
  auto instructionBGTZ() -> void;
  auto instructionBGTZL() -> void;
  auto instructionBLEZ() -> void;
  auto instructionBLEZL() -> void;
  auto instructionBLTZ() -> void;
  auto instructionBLTZAL() -> void;
  auto instructionBLTZALL() -> void;
  auto instructionBLTZL() -> void;
  auto instructionBNE() -> void;
  auto instructionBNEL() -> void;
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
  auto instructionLDC1() -> void;
  auto instructionLDC2() -> void;
  auto instructionLDL() -> void;
  auto instructionLDR() -> void;
  auto instructionLH() -> void;
  auto instructionLHU() -> void;
  auto instructionLUI() -> void;
  auto instructionLL() -> void;
  auto instructionLLD() -> void;
  auto instructionLW() -> void;
  auto instructionLWC1() -> void;
  auto instructionLWC2() -> void;
  auto instructionLWC3() -> void;
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
  auto instructionSDC1() -> void;
  auto instructionSDC2() -> void;
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
  auto instructionSWC1() -> void;
  auto instructionSWC2() -> void;
  auto instructionSWC3() -> void;
  auto instructionSWL() -> void;
  auto instructionSWR() -> void;
  auto instructionSYNC() -> void;
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

  //instructions-cop0.cpp
  auto getCOP0u64(uint) -> u64;
  auto setCOP0u64(uint, u64) -> void;

  auto instructionBC0F() -> void;
  auto instructionBC0FL() -> void;
  auto instructionBC0T() -> void;
  auto instructionBC0TL() -> void;
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

  //instructions-cop1.cpp
  auto instructionBC1F() -> void;
  auto instructionBC1FL() -> void;
  auto instructionBC1T() -> void;
  auto instructionBC1TL() -> void;
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
  auto instructionMFC1() -> void;
  auto instructionMTC1() -> void;

  //serialization.cpp
  auto serializeR4300(serializer&) -> void;

  //exceptions
  enum : uint {
    Break,
    Interrupt,
    InvalidInstruction,
    CoprocessorUnusable,
    FloatingPointError,
    Overflow,
    Syscall,
    Trap,
  };

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
    u64 pc;

    //internal
    maybe<u64> ip;
  } core;

  struct COP0 {
    enum Register : uint {
      Index       =  0,  //programmable pointer into TLB array
      Random      =  1,  //pseudorandom pointer into TLB array (read-only)
      EntryLo0    =  2,  //low half of TLB entry for even virtual address (VPN)
      EntryLo1    =  3,  //low half of TLB entry for odd virtual address (VPN)
      Context     =  4,  //pointer to kernel virtual page table entry (PTE) in 32-bit mode
      PageMask    =  5,  //page size specification
      Wired       =  6,  //number of writed TLB entries
      BadVAddr    =  8,  //last virtual address where an error occurred
      Count       =  9,  //timer count
      EntryHi     = 10,  //high half of TLB entry (including ASID)
      Compare     = 11,  //timer compare value
      Status      = 12,  //operation status setting
      Cause       = 13,  //cause of last exception
      EPC         = 14,  //exception program counter
      PRID        = 15,  //processor revision identifier
      Config      = 16,  //memory system mode setting
      LLAddr      = 17,  //load linked instruction address display
      WatchLo     = 18,  //memory reference trap address low bits
      WatchHi     = 19,  //memory reference trap address high bits
      XContext    = 20,  //pointer to kernel virtual page table entry (PTE) in 64-bit mode
      ParityError = 26,  //(unused in VR4300:) cache parity bits
      CacheError  = 27,  //(unused in VR4300:) cache error and status register
      TagLo       = 28,  //cache tag register low
      TagHi       = 29,  //cache tag register high
      ErrorEPC    = 30,  //error exception program counter
    };

    r64 r[32];
    u32 cr[32];
    bool cf;
    bool llbit;
  } cop0;

  struct COP1 {
    r64 r[32];
    u32 cr[32];
    bool cf[8];
  } cop1;

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);

  #include "disassembler.hpp"
//};
