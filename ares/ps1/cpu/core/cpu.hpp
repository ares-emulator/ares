//{
  using ci32 = const i32;
  using cu32 = const u32;

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
      FP,                              //frame pointer
      RA,                              //return address
    };

    u32 r[32];
    u32 lo;
    u32 hi;
    u32 pc;
  } core;

  struct Delay {
    //load delay slot
    struct {
      u32* target = nullptr;
      u32  source = 0;
      u32  mask = 0;
    } load, fetch, write;

    //branch delay slot
    struct Branch {
      enum : u32 { Step, Take, DelaySlot, Exception };

      auto inDelaySlot() const -> bool { return state == DelaySlot; }
      auto reset() -> void { state = Step; }
      auto take(u32 address) -> void { state = Take; pc = address; }
      auto delaySlot() -> void { state = DelaySlot; }
      auto exception() -> void { state = Exception; }

      u32 pc = 0;
      u32 state = Step;
    } branch;
  } delay;

  auto fetch(u32& target, u32 source, u32 mask = 0) -> void {
    delay.fetch.target = &target;
    delay.fetch.source =  source;
    delay.fetch.mask   =  mask;
  }

  auto write(u32& target, u32 source) -> void {
    delay.write.target = &target;
    delay.write.source =  source;
  }

  auto branch(u32 address) -> void {
    delay.branch.take(address);
  }

  //cpu-instructions.cpp
  auto instructionADD(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionADDI(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionADDIU(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionADDU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionAND(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionANDI(u32& rt, cu32& rs, u16 imm) -> void;
  auto instructionBEQ(cu32& rs, cu32& rt, i16 imm) -> void;
  auto instructionBGEZ(ci32& rs, i16 imm) -> void;
  auto instructionBGEZAL(ci32& rs, i16 imm) -> void;
  auto instructionBGTZ(ci32& rs, i16 imm) -> void;
  auto instructionBLEZ(ci32& rs, i16 imm) -> void;
  auto instructionBLTZ(ci32& rs, i16 imm) -> void;
  auto instructionBLTZAL(ci32& rs, i16 imm) -> void;
  auto instructionBNE(cu32& rs, cu32& rt, i16 imm) -> void;
  auto instructionBREAK() -> void;
  auto instructionDIV(ci32& rs, ci32& rt) -> void;
  auto instructionDIVU(cu32& rs, cu32& rt) -> void;
  auto instructionJ(u32 imm) -> void;
  auto instructionJAL(u32 imm) -> void;
  auto instructionJALR(u32& rd, cu32& rs) -> void;
  auto instructionJR(cu32& rs) -> void;
  auto instructionLB(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionLBU(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionLH(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionLHU(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionLUI(u32& rt, u16 imm) -> void;
  auto instructionLW(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionLWL(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionLWR(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionMFHI(u32& rd) -> void;
  auto instructionMFLO(u32& rd) -> void;
  auto instructionMTHI(cu32& rs) -> void;
  auto instructionMTLO(cu32& rs) -> void;
  auto instructionMULT(ci32& rs, ci32& rt) -> void;
  auto instructionMULTU(cu32& rs, cu32& rt) -> void;
  auto instructionNOR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionOR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionORI(u32& rt, cu32& rs, u16 imm) -> void;
  auto instructionSB(cu32& rt, cu32& rs, i16 imm) -> void;
  auto instructionSH(cu32& rt, cu32& rs, i16 imm) -> void;
  auto instructionSLL(u32& rd, cu32& rt, u8 sa) -> void;
  auto instructionSLLV(u32& rd, cu32& rt, cu32& rs) -> void;
  auto instructionSLT(u32& rd, ci32& rs, ci32& rt) -> void;
  auto instructionSLTI(u32& rt, ci32& rs, i16 imm) -> void;
  auto instructionSLTIU(u32& rt, cu32& rs, i16 imm) -> void;
  auto instructionSLTU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionSRA(u32& rd, ci32& rt, u8 sa) -> void;
  auto instructionSRAV(u32& rd, ci32& rt, cu32& rs) -> void;
  auto instructionSRL(u32& rd, cu32& rt, u8 sa) -> void;
  auto instructionSRLV(u32& rd, cu32& rt, cu32& rs) -> void;
  auto instructionSUB(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionSUBU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionSW(cu32& rt, cu32& rs, i16 imm) -> void;
  auto instructionSWL(cu32& rt, cu32& rs, i16 imm) -> void;
  auto instructionSWR(cu32& rt, cu32& rs, i16 imm) -> void;
  auto instructionSYSCALL() -> void;
  auto instructionXOR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto instructionXORI(u32& rt, cu32& rs, u16 imm) -> void;
//};
