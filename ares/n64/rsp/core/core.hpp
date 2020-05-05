//{
  //core.cpp
  auto powerCore() -> void;

  //memory.cpp
  auto readByte(u32 address) -> maybe<u32>;
  auto readHalf(u32 address) -> maybe<u32>;
  auto readWord(u32 address) -> maybe<u32>;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  union r64 {
    struct {  int32_t order_msb2(i32h, i32); };
    struct { uint32_t order_msb2(u32h, u32); };
    struct {  int64_t i64; };
    struct { uint64_t u64; };
  };

  union v128 {
    struct { uint128_t u128; };
    struct {   __m128i s128; };
    operator __m128i() const { return s128; }
    auto operator=(__m128i value) { s128 = value; }
    auto byte   (uint index) ->  u8& { return  ((u8*)&u128)[15 - index]; }
    auto element(uint index) -> u16& { return ((u16*)&u128)[ 7 - index]; }
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
  auto instructionADDIU() -> void;
  auto instructionADDU() -> void;
  auto instructionAND() -> void;
  auto instructionANDI() -> void;
  auto instructionB(bool take) -> void;
  auto instructionBREAK() -> void;
  auto instructionCACHE() -> void;
  auto instructionJ() -> void;
  auto instructionJAL() -> void;
  auto instructionJALR() -> void;
  auto instructionJR() -> void;
  auto instructionLB() -> void;
  auto instructionLBU() -> void;
  auto instructionLH() -> void;
  auto instructionLHU() -> void;
  auto instructionLUI() -> void;
  auto instructionLW() -> void;
  auto instructionNOR() -> void;
  auto instructionOR() -> void;
  auto instructionORI() -> void;
  auto instructionSB() -> void;
  auto instructionSH() -> void;
  auto instructionSLL() -> void;
  auto instructionSLLV() -> void;
  auto instructionSLTIU() -> void;
  auto instructionSLTU() -> void;
  auto instructionSRA() -> void;
  auto instructionSRAV() -> void;
  auto instructionSRL() -> void;
  auto instructionSRLV() -> void;
  auto instructionSUBU() -> void;
  auto instructionSW() -> void;
  auto instructionXOR() -> void;
  auto instructionXORI() -> void;

  //instructions-cop0.cpp
  auto instructionCFC0() -> void;
  auto instructionCTC0() -> void;
  auto instructionMFC0() -> void;
  auto instructionMTC0() -> void;

  //instructions-cop2.cpp
  auto instructionCFC2() -> void;
  auto instructionCTC2() -> void;
  auto instructionLQV() -> void;
  auto instructionLTV() -> void;
  auto instructionLWC2() -> void;
  auto instructionMFC2() -> void;
  auto instructionMTC2() -> void;
  auto instructionSQV() -> void;
  auto instructionSTV() -> void;
  auto instructionSWC2() -> void;
  auto instructionSWV() -> void;
  auto instructionVABS() -> void;
  auto instructionVADD() -> void;
  auto instructionVADDC() -> void;
  auto instructionVAND() -> void;
  auto instructionVCL() -> void;
  auto instructionVCH() -> void;
  auto instructionVCR() -> void;
  auto instructionVEQ() -> void;
  auto instructionVGE() -> void;
  auto instructionVLT() -> void;
  auto instructionVMACF() -> void;
  auto instructionVMACQ() -> void;
  auto instructionVMACU() -> void;
  auto instructionVMADH() -> void;
  auto instructionVMADL() -> void;
  auto instructionVMADM() -> void;
  auto instructionVMADN() -> void;
  auto instructionVMOV() -> void;
  auto instructionVMRG() -> void;
  auto instructionVMUDH() -> void;
  auto instructionVMUDL() -> void;
  auto instructionVMUDM() -> void;
  auto instructionVMUDN() -> void;
  auto instructionVMULF() -> void;
  auto instructionVMULQ() -> void;
  auto instructionVMULU() -> void;
  auto instructionVNAND() -> void;
  auto instructionVNE() -> void;
  auto instructionVNOP() -> void;
  auto instructionVNOR() -> void;
  auto instructionVNXOR() -> void;
  auto instructionVOR() -> void;
  auto instructionVRCP() -> void;
  auto instructionVRCPH() -> void;
  auto instructionVRCPL() -> void;
  auto instructionVRNDN() -> void;
  auto instructionVRNDP() -> void;
  auto instructionVRSQ() -> void;
  auto instructionVRSQH() -> void;
  auto instructionVRSQL() -> void;
  auto instructionVSAR() -> void;
  auto instructionVSUB() -> void;
  auto instructionVSUBC() -> void;
  auto instructionVXOR() -> void;

  //serialization.cpp
  auto serializeCore(serializer&) -> void;

  //exceptions
  enum : uint {
    Break,
    Interrupt,
    InvalidInstruction,
    CoprocessorUnusable,
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
      R0, AT, V0, V1, A0, A1, A2, A3,
      T0, T1, T2, T3, T4, T5, T6, T7,
      S0, S1, S2, S3, S4, S5, S6, S7,
      T8, T9, K0, K1, GP, SP, S8, RA,
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
      Status = 12,
      Cause  = 13,
      LLAddr = 17,
    };

    r64 r[32];
    u32 cr[32];
    bool cf;
    bool llbit;
  } cop0;

  struct COP2 {
    v128 r[32];
    v128 acch, accm, accl;
    v128 vcoh, vcol;  //16-bit little endian
    v128 vcch, vccl;  //16-bit little endian
    v128 vce;         // 8-bit little endian
  } cop2;

  static constexpr v128 zero{0};
  static constexpr v128 invert{u128(0) - 1};

  #include "disassembler.hpp"
//};
