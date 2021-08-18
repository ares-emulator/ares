//Sharp SM83

//the Game Boy SoC is commonly referred to as the Sharp LR35902
//SM83 is most likely the internal CPU core, based on strong datasheet similarities
//as such, this CPU core could serve as a foundation for any SM83xx SoC

#pragma once

namespace ares {

struct SM83 {
  virtual auto stoppable() -> bool = 0;
  virtual auto stop() -> void = 0;
  virtual auto halt() -> void = 0;
  virtual auto idle() -> void = 0;
  virtual auto read(n16 address) -> n8 = 0;
  virtual auto write(n16 address, n8 data) -> void = 0;
  virtual auto haltBugTrigger() -> void = 0;

  //sm83.cpp
  auto power() -> void;

  //instruction.cpp
  auto instruction() -> void;
  auto instructionCB() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  virtual auto readDebugger(n16 address) -> n8 { return 0; }
  noinline auto disassembleInstruction(maybe<n16> pc = {}) -> string;
  noinline auto disassembleContext() -> string;

  //memory.cpp
  auto operand() -> n8;
  auto operands() -> n16;
  auto load(n16 address) -> n16;
  auto store(n16 address, n16 data) -> void;
  auto pop() -> n16;
  auto push(n16 data) -> void;

  //algorithms.cpp
  auto ADD(n8, n8, bool = 0) -> n8;
  auto AND(n8, n8) -> n8;
  auto BIT(n3, n8) -> void;
  auto CP(n8, n8) -> void;
  auto DEC(n8) -> n8;
  auto INC(n8) -> n8;
  auto OR(n8, n8) -> n8;
  auto RL(n8) -> n8;
  auto RLC(n8) -> n8;
  auto RR(n8) -> n8;
  auto RRC(n8) -> n8;
  auto SLA(n8) -> n8;
  auto SRA(n8) -> n8;
  auto SRL(n8) -> n8;
  auto SUB(n8, n8, bool = 0) -> n8;
  auto SWAP(n8) -> n8;
  auto XOR(n8, n8) -> n8;

  //instructions.cpp
  auto instructionADC_Direct_Data(n8&) -> void;
  auto instructionADC_Direct_Direct(n8&, n8&) -> void;
  auto instructionADC_Direct_Indirect(n8&, n16&) -> void;
  auto instructionADD_Direct_Data(n8&) -> void;
  auto instructionADD_Direct_Direct(n8&, n8&) -> void;
  auto instructionADD_Direct_Direct(n16&, n16&) -> void;
  auto instructionADD_Direct_Indirect(n8&, n16&) -> void;
  auto instructionADD_Direct_Relative(n16&) -> void;
  auto instructionAND_Direct_Data(n8&) -> void;
  auto instructionAND_Direct_Direct(n8&, n8&) -> void;
  auto instructionAND_Direct_Indirect(n8&, n16&) -> void;
  auto instructionBIT_Index_Direct(n3, n8&) -> void;
  auto instructionBIT_Index_Indirect(n3, n16&) -> void;
  auto instructionCALL_Condition_Address(bool) -> void;
  auto instructionCCF() -> void;
  auto instructionCP_Direct_Data(n8&) -> void;
  auto instructionCP_Direct_Direct(n8&, n8&) -> void;
  auto instructionCP_Direct_Indirect(n8&, n16&) -> void;
  auto instructionCPL() -> void;
  auto instructionDAA() -> void;
  auto instructionDEC_Direct(n8&) -> void;
  auto instructionDEC_Direct(n16&) -> void;
  auto instructionDEC_Indirect(n16&) -> void;
  auto instructionDI() -> void;
  auto instructionEI() -> void;
  auto instructionHALT() -> void;
  auto instructionINC_Direct(n8&) -> void;
  auto instructionINC_Direct(n16&) -> void;
  auto instructionINC_Indirect(n16&) -> void;
  auto instructionJP_Condition_Address(bool) -> void;
  auto instructionJP_Direct(n16&) -> void;
  auto instructionJR_Condition_Relative(bool) -> void;
  auto instructionLD_Address_Direct(n8&) -> void;
  auto instructionLD_Address_Direct(n16&) -> void;
  auto instructionLD_Direct_Address(n8&) -> void;
  auto instructionLD_Direct_Data(n8&) -> void;
  auto instructionLD_Direct_Data(n16&) -> void;
  auto instructionLD_Direct_Direct(n8&, n8&) -> void;
  auto instructionLD_Direct_Direct(n16&, n16&) -> void;
  auto instructionLD_Direct_DirectRelative(n16&, n16&) -> void;
  auto instructionLD_Direct_Indirect(n8&, n16&) -> void;
  auto instructionLD_Direct_IndirectDecrement(n8&, n16&) -> void;
  auto instructionLD_Direct_IndirectIncrement(n8&, n16&) -> void;
  auto instructionLD_Indirect_Data(n16&) -> void;
  auto instructionLD_Indirect_Direct(n16&, n8&) -> void;
  auto instructionLD_IndirectDecrement_Direct(n16&, n8&) -> void;
  auto instructionLD_IndirectIncrement_Direct(n16&, n8&) -> void;
  auto instructionLDH_Address_Direct(n8&) -> void;
  auto instructionLDH_Direct_Address(n8&) -> void;
  auto instructionLDH_Direct_Indirect(n8&, n8&) -> void;
  auto instructionLDH_Indirect_Direct(n8&, n8&) -> void;
  auto instructionNOP() -> void;
  auto instructionOR_Direct_Data(n8&) -> void;
  auto instructionOR_Direct_Direct(n8&, n8&) -> void;
  auto instructionOR_Direct_Indirect(n8&, n16&) -> void;
  auto instructionPOP_Direct(n16&) -> void;
  auto instructionPOP_Direct_AF(n16&) -> void;
  auto instructionPUSH_Direct(n16&) -> void;
  auto instructionRES_Index_Direct(n3, n8&) -> void;
  auto instructionRES_Index_Indirect(n3, n16&) -> void;
  auto instructionRET() -> void;
  auto instructionRET_Condition(bool) -> void;
  auto instructionRETI() -> void;
  auto instructionRL_Direct(n8&) -> void;
  auto instructionRL_Indirect(n16&) -> void;
  auto instructionRLA() -> void;
  auto instructionRLC_Direct(n8&) -> void;
  auto instructionRLC_Indirect(n16&) -> void;
  auto instructionRLCA() -> void;
  auto instructionRR_Direct(n8&) -> void;
  auto instructionRR_Indirect(n16&) -> void;
  auto instructionRRA() -> void;
  auto instructionRRC_Direct(n8&) -> void;
  auto instructionRRC_Indirect(n16&) -> void;
  auto instructionRRCA() -> void;
  auto instructionRST_Implied(n8) -> void;
  auto instructionSBC_Direct_Data(n8&) -> void;
  auto instructionSBC_Direct_Direct(n8&, n8&) -> void;
  auto instructionSBC_Direct_Indirect(n8&, n16&) -> void;
  auto instructionSCF() -> void;
  auto instructionSET_Index_Direct(n3, n8&) -> void;
  auto instructionSET_Index_Indirect(n3, n16&) -> void;
  auto instructionSLA_Direct(n8&) -> void;
  auto instructionSLA_Indirect(n16&) -> void;
  auto instructionSRA_Direct(n8&) -> void;
  auto instructionSRA_Indirect(n16&) -> void;
  auto instructionSRL_Direct(n8&) -> void;
  auto instructionSRL_Indirect(n16&) -> void;
  auto instructionSUB_Direct_Data(n8&) -> void;
  auto instructionSUB_Direct_Direct(n8&, n8&) -> void;
  auto instructionSUB_Direct_Indirect(n8&, n16&) -> void;
  auto instructionSWAP_Direct(n8& data) -> void;
  auto instructionSWAP_Indirect(n16& address) -> void;
  auto instructionSTOP() -> void;
  auto instructionXOR_Direct_Data(n8&) -> void;
  auto instructionXOR_Direct_Direct(n8&, n8&) -> void;
  auto instructionXOR_Direct_Indirect(n8&, n16&) -> void;

  struct Registers {
    union Pair {
      Pair() : word(0) {}
      n16 word;
      struct Byte { n8 order_msb2(hi, lo); } byte;
    };

    Pair af;
    Pair bc;
    Pair de;
    Pair hl;
    Pair sp;
    Pair pc;

    n1 ei;
    n1 halt;
    n1 stop;
    n1 ime;
    
    // set if halt bug occurs
    n1 haltBug;
  } r;

  //disassembler.cpp
  auto disassembleOpcode(n16 pc) -> string;
  auto disassembleOpcodeCB(n16 pc) -> string;
};

}
