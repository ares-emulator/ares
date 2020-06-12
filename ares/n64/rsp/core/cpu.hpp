//{
  union r32 {
    struct {  int32_t i32; };
    struct { uint32_t u32; };
  };
  using cr32 = const r32;

  struct Core {
    enum Register : uint {
      R0, AT, V0, V1, A0, A1, A2, A3,
      T0, T1, T2, T3, T4, T5, T6, T7,
      S0, S1, S2, S3, S4, S5, S6, S7,
      T8, T9, K0, K1, GP, SP, S8, RA,
    };

    r32 r[32];
    u32 pc;
  } core;

  struct Branch {
    enum : u32 { Step, Take, DelaySlot, Halt };

    auto inDelaySlot() const -> bool { return state == DelaySlot; }
    auto reset() -> void { state = Step; }
    auto take(u32 address) -> void { state = Take; pc = address; }
    auto delaySlot() -> void { state = DelaySlot; }
    auto halt() -> void { state = Halt; }

    u64 pc;
    u32 state;
  } branch;

  //cpu-instructions.cpp
  auto instructionADDIU(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionADDU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionAND(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionANDI(r32& rt, cr32& rs, u16 imm) -> void;
  auto instructionBEQ(cr32& rs, cr32& rt, i16 imm) -> void;
  auto instructionBGEZ(cr32& rs, i16 imm) -> void;
  auto instructionBGEZAL(cr32& rs, i16 imm) -> void;
  auto instructionBGTZ(cr32& rs, i16 imm) -> void;
  auto instructionBLEZ(cr32& rs, i16 imm) -> void;
  auto instructionBLTZ(cr32& rs, i16 imm) -> void;
  auto instructionBLTZAL(cr32& rs, i16 imm) -> void;
  auto instructionBNE(cr32& rs, cr32& rt, i16 imm) -> void;
  auto instructionBREAK() -> void;
  auto instructionCACHE(u8 cache, u8 operation) -> void;
  auto instructionJ(u32 imm) -> void;
  auto instructionJAL(u32 imm) -> void;
  auto instructionJALR(r32& rd, cr32& rs) -> void;
  auto instructionJR(cr32& rs) -> void;
  auto instructionLB(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionLBU(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionLH(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionLHU(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionLUI(r32& rt, u16 imm) -> void;
  auto instructionLW(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionNOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionORI(r32& rt, cr32& rs, u16 imm) -> void;
  auto instructionSB(cr32& rt, cr32& rs, i16 imm) -> void;
  auto instructionSH(cr32& rt, cr32& rs, i16 imm) -> void;
  auto instructionSLL(r32& rd, cr32& rt, u8 sa) -> void;
  auto instructionSLLV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto instructionSLT(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionSLTI(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionSLTIU(r32& rt, cr32& rs, i16 imm) -> void;
  auto instructionSLTU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionSRA(r32& rd, cr32& rt, u8 sa) -> void;
  auto instructionSRAV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto instructionSRL(r32& rd, cr32& rt, u8 sa) -> void;
  auto instructionSRLV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto instructionSUBU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionSW(cr32& rt, cr32& rs, i16 imm) -> void;
  auto instructionXOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionXORI(r32& rt, cr32& rs, u16 imm) -> void;
//};
