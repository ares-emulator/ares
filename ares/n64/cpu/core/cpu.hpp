//{
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
  using cr64 = const r64;

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
  auto instructionADD(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionADDI(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionADDIU(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionADDU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionAND(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionANDI(r64& rt, cr64& rs, u16 imm) -> void;
  auto instructionBEQ(cr64& rs, cr64& rt, i16 imm) -> void;
  auto instructionBEQL(cr64& rs, cr64& rt, i16 imm) -> void;
  auto instructionBGEZ(cr64& rs, i16 imm) -> void;
  auto instructionBGEZAL(cr64& rs, i16 imm) -> void;
  auto instructionBGEZALL(cr64& rs, i16 imm) -> void;
  auto instructionBGEZL(cr64& rs, i16 imm) -> void;
  auto instructionBGTZ(cr64& rs, i16 imm) -> void;
  auto instructionBGTZL(cr64& rs, i16 imm) -> void;
  auto instructionBLEZ(cr64& rs, i16 imm) -> void;
  auto instructionBLEZL(cr64& rs, i16 imm) -> void;
  auto instructionBLTZ(cr64& rs, i16 imm) -> void;
  auto instructionBLTZAL(cr64& rs, i16 imm) -> void;
  auto instructionBLTZALL(cr64& rs, i16 imm) -> void;
  auto instructionBLTZL(cr64& rs, i16 imm) -> void;
  auto instructionBNE(cr64& rs, cr64& rt, i16 imm) -> void;
  auto instructionBNEL(cr64& rs, cr64& rt, i16 imm) -> void;
  auto instructionBAL(bool take, i16 imm) -> void;
  auto instructionBALL(bool take, i16 imm) -> void;
  auto instructionBREAK() -> void;
  auto instructionCACHE(u8 cache, u8 operation) -> void;
  auto instructionDADD(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionDADDI(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionDADDIU(r64& rt, cr64& rs, i16 imm) -> void;
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
  auto instructionLB(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLBU(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLD(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLDL(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLDR(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLH(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLHU(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLUI(r64& rt, u16 imm) -> void;
  auto instructionLL(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLLD(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLW(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLWL(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLWR(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionLWU(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionMFHI(r64& rd) -> void;
  auto instructionMFLO(r64& rd) -> void;
  auto instructionMTHI(cr64& rs) -> void;
  auto instructionMTLO(cr64& rs) -> void;
  auto instructionMULT(cr64& rs, cr64& rt) -> void;
  auto instructionMULTU(cr64& rs, cr64& rt) -> void;
  auto instructionNOR(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionOR(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionORI(r64& rt, cr64& rs, u16 imm) -> void;
  auto instructionSB(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSC(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSD(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSCD(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSDL(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSDR(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSH(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSLL(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionSLLV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionSLT(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSLTI(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSLTIU(r64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSLTU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSRA(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionSRAV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionSRL(r64& rd, cr64& rt, u8 sa) -> void;
  auto instructionSRLV(r64& rd, cr64& rt, cr64& rs) -> void;
  auto instructionSUB(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSUBU(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionSW(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSWL(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSWR(cr64& rt, cr64& rs, i16 imm) -> void;
  auto instructionSYNC() -> void;
  auto instructionSYSCALL() -> void;
  auto instructionTEQ(cr64& rs, cr64& rt) -> void;
  auto instructionTEQI(cr64& rs, i16 imm) -> void;
  auto instructionTGE(cr64& rs, cr64& rt) -> void;
  auto instructionTGEI(cr64& rs, i16 imm) -> void;
  auto instructionTGEIU(cr64& rs, i16 imm) -> void;
  auto instructionTGEU(cr64& rs, cr64& rt) -> void;
  auto instructionTLT(cr64& rs, cr64& rt) -> void;
  auto instructionTLTI(cr64& rs, i16 imm) -> void;
  auto instructionTLTIU(cr64& rs, i16 imm) -> void;
  auto instructionTLTU(cr64& rs, cr64& rt) -> void;
  auto instructionTNE(cr64& rs, cr64& rt) -> void;
  auto instructionTNEI(cr64& rs, i16 imm) -> void;
  auto instructionXOR(r64& rd, cr64& rs, cr64& rt) -> void;
  auto instructionXORI(r64& rt, cr64& rs, u16 imm) -> void;
//};
