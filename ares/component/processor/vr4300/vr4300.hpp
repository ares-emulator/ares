#pragma once

//NEC VR4300

namespace ares {

struct VR4300 {
  //vr4300.cpp
  auto power() -> void;

  //instruction.cpp
  auto exception(uint type) -> void;
  auto instruction() -> void;
  auto instructionSpecial() -> void;

  //instructions.cpp
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

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleInstructionSpecial() -> vector<string>;
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
    real64_t fpr[32];  //IEEE754
    uint64_t pc;
    uint64_t lo;
    uint64_t hi;
    uint32_t fcr0;
    uint32_t fcr1;
        bool llbit;
  } context;

  uint32 opcode;
};

}
