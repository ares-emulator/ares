#pragma once

namespace ares {

struct GSU {
  #include "registers.hpp"

  virtual auto step(u32 clocks) -> void = 0;

  virtual auto stop() -> void = 0;
  virtual auto color(n8 source) -> n8 = 0;
  virtual auto plot(n8 x, n8 y) -> void = 0;
  virtual auto rpix(n8 x, n8 y) -> n8 = 0;

  virtual auto pipe() -> n8 = 0;
  virtual auto syncROMBuffer() -> void = 0;
  virtual auto readROMBuffer() -> n8 = 0;
  virtual auto syncRAMBuffer() -> void = 0;
  virtual auto readRAMBuffer(n16 address) -> n8 = 0;
  virtual auto writeRAMBuffer(n16 address, n8 data) -> void = 0;
  virtual auto flushCache() -> void = 0;

  virtual auto read(n24 address, n8 data = 0x00) -> n8 = 0;
  virtual auto write(n24 address, n8 data) -> void = 0;

  //gsu.cpp
  auto power() -> void;

  //instructions.cpp
  auto instructionADD_ADC(u32 n) -> void;
  auto instructionALT1() -> void;
  auto instructionALT2() -> void;
  auto instructionALT3() -> void;
  auto instructionAND_BIC(u32 n) -> void;
  auto instructionASR_DIV2() -> void;
  auto instructionBranch(bool c) -> void;
  auto instructionCACHE() -> void;
  auto instructionCOLOR_CMODE() -> void;
  auto instructionDEC(u32 n) -> void;
  auto instructionFMULT_LMULT() -> void;
  auto instructionFROM_MOVES(u32 n) -> void;
  auto instructionGETB() -> void;
  auto instructionGETC_RAMB_ROMB() -> void;
  auto instructionHIB() -> void;
  auto instructionIBT_LMS_SMS(u32 n) -> void;
  auto instructionINC(u32 n) -> void;
  auto instructionIWT_LM_SM(u32 n) -> void;
  auto instructionJMP_LJMP(u32 n) -> void;
  auto instructionLINK(u32 n) -> void;
  auto instructionLoad(u32 n) -> void;
  auto instructionLOB() -> void;
  auto instructionLOOP() -> void;
  auto instructionLSR() -> void;
  auto instructionMERGE() -> void;
  auto instructionMULT_UMULT(u32 n) -> void;
  auto instructionNOP() -> void;
  auto instructionNOT() -> void;
  auto instructionOR_XOR(u32 n) -> void;
  auto instructionPLOT_RPIX() -> void;
  auto instructionROL() -> void;
  auto instructionROR() -> void;
  auto instructionSBK() -> void;
  auto instructionSEX() -> void;
  auto instructionStore(u32 n) -> void;
  auto instructionSTOP() -> void;
  auto instructionSUB_SBC_CMP(u32 n) -> void;
  auto instructionSWAP() -> void;
  auto instructionTO_MOVE(u32 n) -> void;
  auto instructionWITH(u32 n) -> void;

  //switch.cpp
  auto instruction(n8 opcode) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  noinline auto disassembleInstruction() -> string;
  noinline auto disassembleContext() -> string;

  auto disassembleOpcode(char* output) -> void;
  auto disassembleALT0(char* output) -> void;
  auto disassembleALT1(char* output) -> void;
  auto disassembleALT2(char* output) -> void;
  auto disassembleALT3(char* output) -> void;
};

}
