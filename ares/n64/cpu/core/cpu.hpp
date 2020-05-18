//{
  union cpu64 {
    struct {   int32_t order_msb2(i32h, i32); };
    struct {  uint32_t order_msb2(u32h, u32); };
    struct {   int64_t i64; };
    struct {  uint64_t u64; };
    auto i128() const ->  int128_t { return  (int128_t)i64; }
    auto u128() const -> uint128_t { return (uint128_t)u64; }
  };

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

    cpu64 r[32];
    cpu64 lo;
    cpu64 hi;
      u64 pc;  //program counter
  } core;

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

  //cpu-instructions.cpp
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
//};
