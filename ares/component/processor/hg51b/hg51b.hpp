#pragma once

//Hitachi HG51B S169

namespace ares {

struct HG51B {
  //instruction.cpp
  HG51B();

  //hg51b.cpp
  virtual auto step(u32 clocks) -> void;
  virtual auto isROM(n24 address) -> bool = 0;
  virtual auto isRAM(n24 address) -> bool = 0;
  virtual auto read(n24 address) -> n8 = 0;
  virtual auto write(n24 address, n8 data) -> void = 0;
  virtual auto lock() -> void;
  virtual auto halt() -> void;
  auto wait(n24 address) -> u32;
  auto main() -> void;
  auto execute() -> void;
  auto advance() -> void;
  auto suspend() -> void;
  auto cache() -> bool;
  auto dma() -> void;
  auto running() const -> bool;
  auto busy() const -> bool;

  auto power() -> void;

  //instructions.cpp
  auto push() -> void;
  auto pull() -> void;

  auto algorithmADD(n24 x, n24 y) -> n24;
  auto algorithmAND(n24 x, n24 y) -> n24;
  auto algorithmASR(n24 a, n5 s) -> n24;
  auto algorithmMUL(i24 x, i24 y) -> n48;
  auto algorithmOR(n24 x, n24 y) -> n24;
  auto algorithmROR(n24 a, n5 s) -> n24;
  auto algorithmSHL(n24 a, n5 s) -> n24;
  auto algorithmSHR(n24 a, n5 s) -> n24;
  auto algorithmSUB(n24 x, n24 y) -> n24;
  auto algorithmSX(n24 x) -> n24;
  auto algorithmXNOR(n24 x, n24 y) -> n24;
  auto algorithmXOR(n24 x, n24 y) -> n24;

  auto instructionADD(n7 reg, n5 shift) -> void;
  auto instructionADD(n8 imm, n5 shift) -> void;
  auto instructionAND(n7 reg, n5 shift) -> void;
  auto instructionAND(n8 imm, n5 shift) -> void;
  auto instructionASR(n7 reg) -> void;
  auto instructionASR(n5 imm) -> void;
  auto instructionCLEAR() -> void;
  auto instructionCMP(n7 reg, n5 shift) -> void;
  auto instructionCMP(n8 imm, n5 shift) -> void;
  auto instructionCMPR(n7 reg, n5 shift) -> void;
  auto instructionCMPR(n8 imm, n5 shift) -> void;
  auto instructionHALT() -> void;
  auto instructionINC(n24& reg) -> void;
  auto instructionJMP(n8 data, n1 far, const n1& take) -> void;
  auto instructionJSR(n8 data, n1 far, const n1& take) -> void;
  auto instructionLD(n24& out, n7 reg) -> void;
  auto instructionLD(n15& out, n4 reg) -> void;
  auto instructionLD(n24& out, n8 imm) -> void;
  auto instructionLD(n15& out, n8 imm) -> void;
  auto instructionLDL(n15& out, n8 imm) -> void;
  auto instructionLDH(n15& out, n7 imm) -> void;
  auto instructionMUL(n7 reg) -> void;
  auto instructionMUL(n8 imm) -> void;
  auto instructionNOP() -> void;
  auto instructionOR(n7 reg, n5 shift) -> void;
  auto instructionOR(n8 imm, n5 shift) -> void;
  auto instructionRDRAM(n2 byte, n24& a) -> void;
  auto instructionRDRAM(n2 byte, n8 imm) -> void;
  auto instructionRDROM(n24& reg) -> void;
  auto instructionRDROM(n10 imm) -> void;
  auto instructionROR(n7 reg) -> void;
  auto instructionROR(n5 imm) -> void;
  auto instructionRTS() -> void;
  auto instructionSHL(n7 reg) -> void;
  auto instructionSHL(n5 imm) -> void;
  auto instructionSHR(n7 reg) -> void;
  auto instructionSHR(n5 imm) -> void;
  auto instructionSKIP(n1 take, const n1& flag) -> void;
  auto instructionST(n7 reg, n24& in) -> void;
  auto instructionSUB(n7 reg, n5 shift) -> void;
  auto instructionSUB(n8 imm, n5 shift) -> void;
  auto instructionSUBR(n7 reg, n5 shift) -> void;
  auto instructionSUBR(n8 imm, n5 shift) -> void;
  auto instructionSWAP(n24& a, n4 reg) -> void;
  auto instructionSXB() -> void;
  auto instructionSXW() -> void;
  auto instructionWAIT() -> void;
  auto instructionWRRAM(n2 byte, n24& a) -> void;
  auto instructionWRRAM(n2 byte, n8 imm) -> void;
  auto instructionXNOR(n7 reg, n5 shift) -> void;
  auto instructionXNOR(n8 imm, n5 shift) -> void;
  auto instructionXOR(n7 reg, n5 shift) -> void;
  auto instructionXOR(n8 imm, n5 shift) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n16 programRAM[2][256];  //instruction cache
  n24 dataROM[1024];
  n8  dataRAM[3072];

  //registers.cpp
  auto readRegister(n7 address) -> n24;
  auto writeRegister(n7 address, n24 data) -> void;

  //disassembler.cpp
  noinline auto disassembleInstruction(maybe<n15> pb = {}, maybe<n8> pc = {}) -> string;
  noinline auto disassembleContext() -> string;

protected:
  struct Registers {
    n15 pb;  //program bank
    n8  pc;  //program counter

    b1 n;  //negative
    b1 z;  //zero
    b1 c;  //carry
    b1 v;  //overflow
    b1 i;  //interrupt

    n24 a;        //accumulator
    n15 p;        //page register
    n48 mul;      //multiplier
    n24 mdr;      //bus memory data register
    n24 rom;      //data ROM data buffer
    n24 ram;      //data RAM data buffer
    n24 mar;      //bus memory address register
    n24 dpr;      //data RAM address pointer
    n24 gpr[16];  //general purpose registers
  } r;

  struct IO {
    n1 lock;
    n1 halt = 1;
    n1 irq;      //0 = enable, 1 = disable
    n1 rom = 1;  //0 = 2 ROMs, 1 = 1 ROM
    n8 vector[32];

    struct Wait {
      n3 rom = 3;
      n3 ram = 3;
    } wait;

    struct Suspend {
      n1 enable;
      n8 duration;
    } suspend;

    struct Cache {
      n1  enable;
      n1  page;
      n1  lock[2];
      n24 address[2];  //cache address is in bytes; so 24-bit
      n24 base;        //base address is also in bytes
      n15 pb;
      n8  pc;
    } cache;

    struct DMA {
      n1  enable;
      n24 source;
      n24 target;
      n16 length;
    } dma;

    struct Bus {
      n1  enable;
      n1  reading;
      n1  writing;
      n4  pending;
      n24 address;
    } bus;
  } io;

  n23 stack[8];
  function<void   ()> instructionTable[65536];
  function<string ()> disassembleTable[65536];

  auto disassembleADD(n7 reg, n5 shift) -> string;
  auto disassembleADD(n8 imm, n5 shift) -> string;
  auto disassembleAND(n7 reg, n5 shift) -> string;
  auto disassembleAND(n8 imm, n5 shift) -> string;
  auto disassembleASR(n7 reg) -> string;
  auto disassembleASR(n5 imm) -> string;
  auto disassembleCLEAR() -> string;
  auto disassembleCMP(n7 reg, n5 shift) -> string;
  auto disassembleCMP(n8 imm, n5 shift) -> string;
  auto disassembleCMPR(n7 reg, n5 shift) -> string;
  auto disassembleCMPR(n8 imm, n5 shift) -> string;
  auto disassembleHALT() -> string;
  auto disassembleINC(n24& reg) -> string;
  auto disassembleJMP(n8 data, n1 far, const n1& take) -> string;
  auto disassembleJSR(n8 data, n1 far, const n1& take) -> string;
  auto disassembleLD(n24& out, n7 reg) -> string;
  auto disassembleLD(n15& out, n4 reg) -> string;
  auto disassembleLD(n24& out, n8 imm) -> string;
  auto disassembleLD(n15& out, n8 imm) -> string;
  auto disassembleLDL(n15& out, n8 imm) -> string;
  auto disassembleLDH(n15& out, n7 imm) -> string;
  auto disassembleMUL(n7 reg) -> string;
  auto disassembleMUL(n8 imm) -> string;
  auto disassembleNOP() -> string;
  auto disassembleOR(n7 reg, n5 shift) -> string;
  auto disassembleOR(n8 imm, n5 shift) -> string;
  auto disassembleRDRAM(n2 byte, n24& a) -> string;
  auto disassembleRDRAM(n2 byte, n8 imm) -> string;
  auto disassembleRDROM(n24& reg) -> string;
  auto disassembleRDROM(n10 imm) -> string;
  auto disassembleROR(n7 reg) -> string;
  auto disassembleROR(n5 imm) -> string;
  auto disassembleRTS() -> string;
  auto disassembleSHL(n7 reg) -> string;
  auto disassembleSHL(n5 imm) -> string;
  auto disassembleSHR(n7 reg) -> string;
  auto disassembleSHR(n5 imm) -> string;
  auto disassembleSKIP(n1 take, const n1& flag) -> string;
  auto disassembleST(n7 reg, n24& in) -> string;
  auto disassembleSUB(n7 reg, n5 shift) -> string;
  auto disassembleSUB(n8 imm, n5 shift) -> string;
  auto disassembleSUBR(n7 reg, n5 shift) -> string;
  auto disassembleSUBR(n8 imm, n5 shift) -> string;
  auto disassembleSWAP(n24& a, n4 reg) -> string;
  auto disassembleSXB() -> string;
  auto disassembleSXW() -> string;
  auto disassembleWAIT() -> string;
  auto disassembleWRRAM(n2 byte, n24& a) -> string;
  auto disassembleWRRAM(n2 byte, n8 imm) -> string;
  auto disassembleXNOR(n7 reg, n5 shift) -> string;
  auto disassembleXNOR(n8 imm, n5 shift) -> string;
  auto disassembleXOR(n7 reg, n5 shift) -> string;
  auto disassembleXOR(n8 imm, n5 shift) -> string;
};

}
