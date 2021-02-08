//NEC uPD7725
//NEC uPD96050

#pragma once

namespace ares {

struct uPD96050 {
  auto power() -> void;
  auto exec() -> void;
  auto serialize(serializer&) -> void;

  auto execOP(n24 opcode) -> void;
  auto execRT(n24 opcode) -> void;
  auto execJP(n24 opcode) -> void;
  auto execLD(n24 opcode) -> void;

  auto readSR() -> n8;
  auto writeSR(n8 data) -> void;

  auto readDR() -> n8;
  auto writeDR(n8 data) -> void;

  auto readDP(n12 address) -> n8;
  auto writeDP(n12 address, n8 data) -> void;

  //disassembler.cpp
  noinline auto disassembleInstruction(maybe<n14> ip = {}) -> string;
  noinline auto disassembleContext() -> string;

  enum class Revision : u32 { uPD7725, uPD96050 } revision;
  n24 programROM[16384];
  n16 dataROM[2048];
  n16 dataRAM[2048];

  struct Flag {
    operator u32() const {
      return ov0 << 0 | ov1 << 1 | z << 2 | c << 3 | s0 << 4 | s1 << 5;
    }

    auto operator=(n16 data) -> Flag& {
      ov0 = data.bit(0);
      ov1 = data.bit(1);
      z   = data.bit(2);
      c   = data.bit(3);
      s0  = data.bit(4);
      s1  = data.bit(5);
      return *this;
    }

    auto serialize(serializer&) -> void;

    boolean ov0;  //overflow 0
    boolean ov1;  //overflow 1
    boolean z;    //zero
    boolean c;    //carry
    boolean s0;   //sign 0
    boolean s1;   //sign 1
  };

  struct Status {
    operator u32() const {
      bool _drs = drs & !drc;  //when DRC=1, DRS=0
      return p0 << 0 | p1 << 1 | ei << 7 | sic << 8 | soc << 9 | drc << 10
           | dma << 11 | _drs << 12 | usf0 << 13 | usf1 << 14 | rqm << 15;
    }

    auto operator=(n16 data) -> Status& {
      p0   = data.bit( 0);
      p1   = data.bit( 1);
      ei   = data.bit( 7);
      sic  = data.bit( 8);
      soc  = data.bit( 9);
      drc  = data.bit(10);
      dma  = data.bit(11);
      drs  = data.bit(12);
      usf0 = data.bit(13);
      usf1 = data.bit(14);
      rqm  = data.bit(15);
      return *this;
    }

    auto serialize(serializer&) -> void;

    boolean p0;    //output port 0
    boolean p1;    //output port 1
    boolean ei;    //enable interrupts
    boolean sic;   //serial input control  (0 = 16-bit; 1 = 8-bit)
    boolean soc;   //serial output control (0 = 16-bit; 1 = 8-bit)
    boolean drc;   //data register size    (0 = 16-bit; 1 = 8-bit)
    boolean dma;   //data register DMA mode
    boolean drs;   //data register status  (1 = active; 0 = stopped)
    boolean usf0;  //user flag 0
    boolean usf1;  //user flag 1
    boolean rqm;   //request for master (=1 on internal access; =0 on external access)

    //internal
    boolean siack;  //serial input acknowledge
    boolean soack;  //serial output acknowledge
  };

  struct Registers {
    auto serialize(serializer&) -> void;

    n16 stack[16];       //LIFO
    VariadicNatural pc;  //program counter
    VariadicNatural rp;  //ROM pointer
    VariadicNatural dp;  //data pointer
    n4 sp;               //stack pointer
    n16 si;              //serial input
    n16 so;              //serial output
    i16 k;
    i16 l;
    i16 m;
    i16 n;
    i16 a;               //accumulator
    i16 b;               //accumulator
    n16 tr;              //temporary register
    n16 trb;             //temporary register
    n16 dr;              //data register
    Status sr;           //status register
  } regs;

  struct Flags {
    Flag a;
    Flag b;
  } flags;
};

}
