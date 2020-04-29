#pragma once

//NEC VR4300

namespace ares {

struct VR4300 {
  virtual auto instructionCOP0() -> void {}
  virtual auto instructionCOP1() -> void {}
  virtual auto instructionCOP2() -> void {}
  virtual auto instructionCOP3() -> void {}

  //vr4300.cpp
  auto power() -> void;

  //instruction.cpp
  auto exception(uint type) -> void;
  auto instruction() -> void;
  auto instructionSpecial() -> void;
  auto instructionRegisterImmediate() -> void;

  //instructions-special.cpp
  auto instructionADD() -> void;
  auto instructionADDU() -> void;
  auto instructionAND() -> void;
  auto instructionBREAK() -> void;
  auto instructionDADD() -> void;
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
  auto instructionJALR() -> void;
  auto instructionJR() -> void;
  auto instructionMFHI() -> void;
  auto instructionMFLO() -> void;
  auto instructionMTHI() -> void;
  auto instructionMTLO() -> void;
  auto instructionMULT() -> void;
  auto instructionMULTU() -> void;
  auto instructionNOR() -> void;
  auto instructionOR() -> void;
  auto instructionSLL() -> void;
  auto instructionSLLV() -> void;
  auto instructionSLT() -> void;
  auto instructionSLTU() -> void;
  auto instructionSRA() -> void;
  auto instructionSRAV() -> void;
  auto instructionSRL() -> void;
  auto instructionSRLV() -> void;
  auto instructionSUB() -> void;
  auto instructionSUBU() -> void;
  auto instructionSYNC() -> void;
  auto instructionSYSCALL() -> void;
  auto instructionTEQ() -> void;
  auto instructionTGE() -> void;
  auto instructionTGEU() -> void;
  auto instructionTLT() -> void;
  auto instructionTLTU() -> void;
  auto instructionTNE() -> void;
  auto instructionXOR() -> void;

  //instructions-regimm.cpp
  auto instructionBGEZ() -> void;
  auto instructionBGEZAL() -> void;
  auto instructionBGEZALL() -> void;
  auto instructionBGEZL() -> void;
  auto instructionBLTZ() -> void;
  auto instructionBLTZAL() -> void;
  auto instructionBLTZALL() -> void;
  auto instructionBLTZL() -> void;
  auto instructionTEQI() -> void;
  auto instructionTGEI() -> void;
  auto instructionTGEIU() -> void;
  auto instructionTLTI() -> void;
  auto instructionTLTIU() -> void;
  auto instructionTNEI() -> void;

  //instructions.cpp
  auto instructionADDI() -> void;
  auto instructionADDIU() -> void;
  auto instructionANDI() -> void;
  auto instructionBEQ() -> void;
  auto instructionBEQL() -> void;
  auto instructionBLEZ() -> void;
  auto instructionBLEZL() -> void;
  auto instructionBNE() -> void;
  auto instructionBNEL() -> void;
  auto instructionBGTZ() -> void;
  auto instructionBGTZL() -> void;
  auto instructionDADDI() -> void;
  auto instructionDADDIU() -> void;
  auto instructionJ() -> void;
  auto instructionJAL() -> void;
  auto instructionLDL() -> void;
  auto instructionLDR() -> void;
  auto instructionLUI() -> void;
  auto instructionORI() -> void;
  auto instructionSLTI() -> void;
  auto instructionSLTIU() -> void;
  auto instructionXORI() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleInstructionDecode() -> vector<string>;
  auto disassembleInstructionSpecial() -> vector<string>;
  auto disassembleInstructionRegisterImmediate() -> vector<string>;
  auto disassembleContext() -> string;

  //exceptions
  enum : uint {
    Break,
    InvalidInstruction,
    Overflow,
    Syscall,
    Trap,
  };

  struct Context {
    uint64_t gpr[32];
    uint64_t lo;
    uint64_t hi;
    uint64_t pc;

    //internal
    uint64_t ip;
  } r;

  uint32 opcode;
};

}
