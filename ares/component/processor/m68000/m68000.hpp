#pragma once

//Motorola MC68000

namespace ares {

struct M68000 {
  virtual auto idle(u32 clocks) -> void = 0;
  virtual auto wait(u32 clocks) -> void = 0;
  virtual auto read(n1 upper, n1 lower, n24 address, n16 data = 0) -> n16 = 0;
  virtual auto write(n1 upper, n1 lower, n24 address, n16 data) -> void = 0;
  virtual auto lockable() -> bool { return true; }

  auto ird() const -> n16 { return r.ird; }

  enum : bool { User, Supervisor };
  enum : u32  { Byte, Word, Long };
  enum : bool { Reverse = 1, Extend = 1, Hold = 1, Fast = 1 };

  enum : u32 {
    /* 0,n */ DataRegisterDirect,
    /* 1,n */ AddressRegisterDirect,
    /* 2,n */ AddressRegisterIndirect,
    /* 3,n */ AddressRegisterIndirectWithPostIncrement,
    /* 4,n */ AddressRegisterIndirectWithPreDecrement,
    /* 5,n */ AddressRegisterIndirectWithDisplacement,
    /* 6,n */ AddressRegisterIndirectWithIndex,
    /* 7,0 */ AbsoluteShortIndirect,
    /* 7,1 */ AbsoluteLongIndirect,
    /* 7,2 */ ProgramCounterIndirectWithDisplacement,
    /* 7,3 */ ProgramCounterIndirectWithIndex,
    /* 7,4 */ Immediate,
  };

  struct Exception { enum : u32 {
    Illegal,
    DivisionByZero,
    BoundsCheck,
    Overflow,
    Unprivileged,

    Trap,
    Interrupt,
  };};

  struct Vector { enum : u32 {
    ResetSP            =  0,  //0x00
    ResetPC            =  1,  //0x04
    BusError           =  2,  //0x08
    AddressError       =  3,  //0x0c
    IllegalInstruction =  4,  //0x10
    DivisionByZero     =  5,  //0x14
    BoundsCheck        =  6,  //0x18
    Overflow           =  7,  //0x1c
    Unprivileged       =  8,  //0x20
    Trace              =  9,  //0x24
    IllegalLineA       = 10,  //0x28
    IllegalLineF       = 11,  //0x2c
    Spurious           = 24,  //0x60
    Level1             = 25,  //0x64
    Level2             = 26,  //0x68
    Level3             = 27,  //0x6c
    Level4             = 28,  //0x70
    Level5             = 29,  //0x74
    Level6             = 30,  //0x78
    Level7             = 31,  //0x7c
    Trap               = 32,  //0x80-0xbc (#0-#15)
  };};

  //m68000.cpp
  M68000();
  auto power() -> void;
  auto supervisor() -> bool;
  auto exception(u32 exception, u32 vector, u32 priority = 0) -> void;
  auto interrupt(u32 vector, u32 priority = 0) -> void;

  //registers.cpp
  struct DataRegister {
    explicit DataRegister(n64 number_) : number(number_) {}
    n3 number;
  };
  template<u32 Size = Long> auto read(DataRegister reg) -> n32;
  template<u32 Size = Long> auto write(DataRegister reg, n32 data) -> void;

  struct AddressRegister {
    explicit AddressRegister(n64 number_) : number(number_) {}
    n3 number;
  };
  template<u32 Size = Long> auto read(AddressRegister reg) -> n32;
  template<u32 Size = Long> auto write(AddressRegister reg, n32 data) -> void;

  auto readCCR() -> n8;
  auto readSR() -> n16;
  auto writeCCR(n8 ccr) -> void;
  auto writeSR(n16 sr) -> void;

  //memory.cpp
  template<u32 Size> auto read(n32 addr) -> n32;
  template<u32 Size, bool Order = 0> auto write(n32 addr, n32 data) -> void;
  template<u32 Size> auto extension() -> n32;
  auto prefetch() -> n16;
  auto prefetched() -> n16;
  template<u32 Size> auto pop() -> n32;
  template<u32 Size> auto push(n32 data) -> void;

  //effective-address.cpp
  struct EffectiveAddress {
    explicit EffectiveAddress(n4 mode_, n3 reg_) : mode(mode_), reg(reg_) {
      if(mode == 7) mode += reg;  //optimization: convert modes {7; 0-4} to {7-11}
    }

    n4 mode;
    n3 reg;

    b1 valid;
    n32 address;
  };

  auto prefetched(EffectiveAddress& ea) -> n32;
  template<u32 Size> auto fetch(EffectiveAddress& ea) -> n32;
  template<u32 Size, bool Hold = 0, bool Fast = 0> auto read(EffectiveAddress& ea) -> n32;
  template<u32 Size, bool Hold = 0> auto write(EffectiveAddress& ea, n32 data) -> void;

  //instruction.cpp
  auto instruction() -> void;

  //traits.cpp
  template<u32 Size> auto bytes() -> u32;
  template<u32 Size> auto bits() -> u32;
  template<u32 Size> auto lsb() -> n32;
  template<u32 Size> auto msb() -> n32;
  template<u32 Size> auto mask() -> n32;
  template<u32 Size> auto clip(n32 data) -> n32;
  template<u32 Size> auto sign(n32 data) -> i32;

  //conditions.cpp
  auto condition(n4 condition) -> bool;

  //algorithms.cpp
  template<u32 Size, bool Extend = false> auto ADD(n32 source, n32 target) -> n32;
  template<u32 Size> auto AND(n32 source, n32 target) -> n32;
  template<u32 Size> auto ASL(n32 result, u32 shift) -> n32;
  template<u32 Size> auto ASR(n32 result, u32 shift) -> n32;
  template<u32 Size> auto CMP(n32 source, n32 target) -> n32;
  template<u32 Size> auto EOR(n32 source, n32 target) -> n32;
  template<u32 Size> auto LSL(n32 result, u32 shift) -> n32;
  template<u32 Size> auto LSR(n32 result, u32 shift) -> n32;
  template<u32 Size> auto OR(n32 source, n32 target) -> n32;
  template<u32 Size> auto ROL(n32 result, u32 shift) -> n32;
  template<u32 Size> auto ROR(n32 result, u32 shift) -> n32;
  template<u32 Size> auto ROXL(n32 result, u32 shift) -> n32;
  template<u32 Size> auto ROXR(n32 result, u32 shift) -> n32;
  template<u32 Size, bool Extend = false> auto SUB(n32 source, n32 target) -> n32;

  //instructions.cpp
                     auto instructionABCD(EffectiveAddress from, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionADD(EffectiveAddress from, DataRegister with) -> void;
  template<u32 Size> auto instructionADD(DataRegister from, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionADDA(EffectiveAddress from, AddressRegister with) -> void;
  template<u32 Size> auto instructionADDI(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionADDQ(n4 immediate, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionADDQ(n4 immediate, AddressRegister with) -> void;
  template<u32 Size> auto instructionADDX(EffectiveAddress with, EffectiveAddress from) -> void;
  template<u32 Size> auto instructionAND(EffectiveAddress from, DataRegister with) -> void;
  template<u32 Size> auto instructionAND(DataRegister from, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionANDI(EffectiveAddress with) -> void;
                     auto instructionANDI_TO_CCR() -> void;
                     auto instructionANDI_TO_SR() -> void;
  template<u32 Size> auto instructionASL(n4 count, DataRegister modify) -> void;
  template<u32 Size> auto instructionASL(DataRegister from, DataRegister modify) -> void;
                     auto instructionASL(EffectiveAddress modify) -> void;
  template<u32 Size> auto instructionASR(n4 count, DataRegister modify) -> void;
  template<u32 Size> auto instructionASR(DataRegister from, DataRegister modify) -> void;
                     auto instructionASR(EffectiveAddress modify) -> void;
                     auto instructionBCC(n4 test, n8 displacement) -> void;
  template<u32 Size> auto instructionBCHG(DataRegister bit, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionBCHG(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionBCLR(DataRegister bit, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionBCLR(EffectiveAddress with) -> void;
                     auto instructionBRA(n8 displacement) -> void;
  template<u32 Size> auto instructionBSET(DataRegister bit, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionBSET(EffectiveAddress with) -> void;
                     auto instructionBSR(n8 displacement) -> void;
  template<u32 Size> auto instructionBTST(DataRegister bit, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionBTST(EffectiveAddress with) -> void;
                     auto instructionCHK(DataRegister compare, EffectiveAddress maximum) -> void;
  template<u32 Size> auto instructionCLR(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionCMP(EffectiveAddress from, DataRegister with) -> void;
  template<u32 Size> auto instructionCMPA(EffectiveAddress from, AddressRegister with) -> void;
  template<u32 Size> auto instructionCMPI(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionCMPM(EffectiveAddress from, EffectiveAddress with) -> void;
                     auto instructionDBCC(n4 condition, DataRegister with) -> void;
                     auto instructionDIVS(EffectiveAddress from, DataRegister with) -> void;
                     auto instructionDIVU(EffectiveAddress from, DataRegister with) -> void;
  template<u32 Size> auto instructionEOR(DataRegister from, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionEORI(EffectiveAddress with) -> void;
                     auto instructionEORI_TO_CCR() -> void;
                     auto instructionEORI_TO_SR() -> void;
                     auto instructionEXG(DataRegister x, DataRegister y) -> void;
                     auto instructionEXG(AddressRegister x, AddressRegister y) -> void;
                     auto instructionEXG(DataRegister x, AddressRegister y) -> void;
  template<u32 Size> auto instructionEXT(DataRegister with) -> void;
                     auto instructionILLEGAL(n16 code) -> void;
                     auto instructionJMP(EffectiveAddress from) -> void;
                     auto instructionJSR(EffectiveAddress from) -> void;
                     auto instructionLEA(EffectiveAddress from, AddressRegister to) -> void;
                     auto instructionLINK(AddressRegister with) -> void;
  template<u32 Size> auto instructionLSL(n4 count, DataRegister with) -> void;
  template<u32 Size> auto instructionLSL(DataRegister from, DataRegister with) -> void;
                     auto instructionLSL(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionLSR(n4 count, DataRegister with) -> void;
  template<u32 Size> auto instructionLSR(DataRegister from, DataRegister with) -> void;
                     auto instructionLSR(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionMOVE(EffectiveAddress from, EffectiveAddress to) -> void;
  template<u32 Size> auto instructionMOVEA(EffectiveAddress from, AddressRegister to) -> void;
  template<u32 Size> auto instructionMOVEM_TO_MEM(EffectiveAddress to) -> void;
  template<u32 Size> auto instructionMOVEM_TO_REG(EffectiveAddress from) -> void;
  template<u32 Size> auto instructionMOVEP(DataRegister from, EffectiveAddress to) -> void;
  template<u32 Size> auto instructionMOVEP(EffectiveAddress from, DataRegister to) -> void;
                     auto instructionMOVEQ(n8 immediate, DataRegister to) -> void;
                     auto instructionMOVE_FROM_SR(EffectiveAddress to) -> void;
                     auto instructionMOVE_TO_CCR(EffectiveAddress from) -> void;
                     auto instructionMOVE_TO_SR(EffectiveAddress from) -> void;
                     auto instructionMOVE_FROM_USP(AddressRegister to) -> void;
                     auto instructionMOVE_TO_USP(AddressRegister from) -> void;
                     auto instructionMULS(EffectiveAddress from, DataRegister with) -> void;
                     auto instructionMULU(EffectiveAddress from, DataRegister with) -> void;
                     auto instructionNBCD(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionNEG(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionNEGX(EffectiveAddress with) -> void;
                     auto instructionNOP() -> void;
  template<u32 Size> auto instructionNOT(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionOR(EffectiveAddress from, DataRegister with) -> void;
  template<u32 Size> auto instructionOR(DataRegister from, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionORI(EffectiveAddress with) -> void;
                     auto instructionORI_TO_CCR() -> void;
                     auto instructionORI_TO_SR() -> void;
                     auto instructionPEA(EffectiveAddress from) -> void;
                     auto instructionRESET() -> void;
  template<u32 Size> auto instructionROL(n4 count, DataRegister with) -> void;
  template<u32 Size> auto instructionROL(DataRegister from, DataRegister with) -> void;
                     auto instructionROL(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionROR(n4 count, DataRegister with) -> void;
  template<u32 Size> auto instructionROR(DataRegister from, DataRegister with) -> void;
                     auto instructionROR(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionROXL(n4 count, DataRegister with) -> void;
  template<u32 Size> auto instructionROXL(DataRegister from, DataRegister with) -> void;
                     auto instructionROXL(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionROXR(n4 count, DataRegister with) -> void;
  template<u32 Size> auto instructionROXR(DataRegister from, DataRegister with) -> void;
                     auto instructionROXR(EffectiveAddress with) -> void;
                     auto instructionRTE() -> void;
                     auto instructionRTR() -> void;
                     auto instructionRTS() -> void;
                     auto instructionSBCD(EffectiveAddress with, EffectiveAddress from) -> void;
                     auto instructionSCC(n4 test, EffectiveAddress to) -> void;
                     auto instructionSTOP() -> void;
  template<u32 Size> auto instructionSUB(EffectiveAddress from, DataRegister with) -> void;
  template<u32 Size> auto instructionSUB(DataRegister from, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionSUBA(EffectiveAddress from, AddressRegister with) -> void;
  template<u32 Size> auto instructionSUBI(EffectiveAddress with) -> void;
  template<u32 Size> auto instructionSUBQ(n4 immediate, EffectiveAddress with) -> void;
  template<u32 Size> auto instructionSUBQ(n4 immediate, AddressRegister with) -> void;
  template<u32 Size> auto instructionSUBX(EffectiveAddress from, EffectiveAddress with) -> void;
                     auto instructionSWAP(DataRegister with) -> void;
                     auto instructionTAS(EffectiveAddress with) -> void;
                     auto instructionTRAP(n4 vector) -> void;
                     auto instructionTRAPV() -> void;
  template<u32 Size> auto instructionTST(EffectiveAddress from) -> void;
                     auto instructionUNLK(AddressRegister with) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction(n32 pc) -> string;
  auto disassembleContext() -> string;

  struct Registers {
    n32 d[8];  //data registers
    n32 a[8];  //address registers (a7 = s ? ssp : usp)
    n32 sp;    //inactive stack pointer (s ? usp : ssp)
    n32 pc;    //program counter

    bool c;  //carry
    bool v;  //overflow
    bool z;  //zero
    bool n;  //negative
    bool x;  //extend
    n3 i;    //interrupt mask
    bool s;  //supervisor mode
    bool t;  //trace mode

    n16 irc;  //instruction prefetched from external memory
    n16 ir;   //instruction currently being decoded
    n16 ird;  //instruction currently being executed

    bool stop;
    bool reset;
  } r;

  function<void ()> instructionTable[65536];

private:
  //disassembler.cpp
                     auto disassembleABCD(EffectiveAddress from, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleADD(EffectiveAddress from, DataRegister with) -> string;
  template<u32 Size> auto disassembleADD(DataRegister from, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleADDA(EffectiveAddress from, AddressRegister with) -> string;
  template<u32 Size> auto disassembleADDI(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleADDQ(n4 immediate, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleADDQ(n4 immediate, AddressRegister with) -> string;
  template<u32 Size> auto disassembleADDX(EffectiveAddress from, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleAND(EffectiveAddress from, DataRegister with) -> string;
  template<u32 Size> auto disassembleAND(DataRegister from, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleANDI(EffectiveAddress with) -> string;
                     auto disassembleANDI_TO_CCR() -> string;
                     auto disassembleANDI_TO_SR() -> string;
  template<u32 Size> auto disassembleASL(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleASL(DataRegister from, DataRegister with) -> string;
                     auto disassembleASL(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleASR(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleASR(DataRegister from, DataRegister with) -> string;
                     auto disassembleASR(EffectiveAddress with) -> string;
                     auto disassembleBCC(n4 condition, n8 displacement) -> string;
  template<u32 Size> auto disassembleBCHG(DataRegister bit, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleBCHG(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleBCLR(DataRegister bit, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleBCLR(EffectiveAddress with) -> string;
                     auto disassembleBRA(n8 displacement) -> string;
  template<u32 Size> auto disassembleBSET(DataRegister bit, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleBSET(EffectiveAddress with) -> string;
                     auto disassembleBSR(n8 displacement) -> string;
  template<u32 Size> auto disassembleBTST(DataRegister bit, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleBTST(EffectiveAddress with) -> string;
                     auto disassembleCHK(DataRegister compare, EffectiveAddress maximum) -> string;
  template<u32 Size> auto disassembleCLR(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleCMP(EffectiveAddress from, DataRegister with) -> string;
  template<u32 Size> auto disassembleCMPA(EffectiveAddress from, AddressRegister with) -> string;
  template<u32 Size> auto disassembleCMPI(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleCMPM(EffectiveAddress from, EffectiveAddress with) -> string;
                     auto disassembleDBCC(n4 test, DataRegister with) -> string;
                     auto disassembleDIVS(EffectiveAddress from, DataRegister with) -> string;
                     auto disassembleDIVU(EffectiveAddress from, DataRegister with) -> string;
  template<u32 Size> auto disassembleEOR(DataRegister from, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleEORI(EffectiveAddress with) -> string;
                     auto disassembleEORI_TO_CCR() -> string;
                     auto disassembleEORI_TO_SR() -> string;
                     auto disassembleEXG(DataRegister x, DataRegister y) -> string;
                     auto disassembleEXG(AddressRegister x, AddressRegister y) -> string;
                     auto disassembleEXG(DataRegister x, AddressRegister y) -> string;
  template<u32 Size> auto disassembleEXT(DataRegister with) -> string;
                     auto disassembleILLEGAL(n16 code) -> string;
                     auto disassembleJMP(EffectiveAddress from) -> string;
                     auto disassembleJSR(EffectiveAddress from) -> string;
                     auto disassembleLEA(EffectiveAddress from, AddressRegister to) -> string;
                     auto disassembleLINK(AddressRegister with) -> string;
  template<u32 Size> auto disassembleLSL(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleLSL(DataRegister from, DataRegister with) -> string;
                     auto disassembleLSL(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleLSR(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleLSR(DataRegister from, DataRegister with) -> string;
                     auto disassembleLSR(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleMOVE(EffectiveAddress from, EffectiveAddress to) -> string;
  template<u32 Size> auto disassembleMOVEA(EffectiveAddress from, AddressRegister to) -> string;
  template<u32 Size> auto disassembleMOVEM_TO_MEM(EffectiveAddress to) -> string;
  template<u32 Size> auto disassembleMOVEM_TO_REG(EffectiveAddress from) -> string;
  template<u32 Size> auto disassembleMOVEP(DataRegister from, EffectiveAddress to) -> string;
  template<u32 Size> auto disassembleMOVEP(EffectiveAddress from, DataRegister to) -> string;
                     auto disassembleMOVEQ(n8 immediate, DataRegister to) -> string;
                     auto disassembleMOVE_FROM_SR(EffectiveAddress to) -> string;
                     auto disassembleMOVE_TO_CCR(EffectiveAddress from) -> string;
                     auto disassembleMOVE_TO_SR(EffectiveAddress from) -> string;
                     auto disassembleMOVE_FROM_USP(AddressRegister to) -> string;
                     auto disassembleMOVE_TO_USP(AddressRegister from) -> string;
                     auto disassembleMULS(EffectiveAddress from, DataRegister with) -> string;
                     auto disassembleMULU(EffectiveAddress from, DataRegister with) -> string;
                     auto disassembleNBCD(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleNEG(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleNEGX(EffectiveAddress with) -> string;
                     auto disassembleNOP() -> string;
  template<u32 Size> auto disassembleNOT(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleOR(EffectiveAddress from, DataRegister with) -> string;
  template<u32 Size> auto disassembleOR(DataRegister from, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleORI(EffectiveAddress with) -> string;
                     auto disassembleORI_TO_CCR() -> string;
                     auto disassembleORI_TO_SR() -> string;
                     auto disassemblePEA(EffectiveAddress from) -> string;
                     auto disassembleRESET() -> string;
  template<u32 Size> auto disassembleROL(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleROL(DataRegister from, DataRegister with) -> string;
                     auto disassembleROL(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleROR(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleROR(DataRegister from, DataRegister with) -> string;
                     auto disassembleROR(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleROXL(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleROXL(DataRegister from, DataRegister with) -> string;
                     auto disassembleROXL(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleROXR(n4 count, DataRegister with) -> string;
  template<u32 Size> auto disassembleROXR(DataRegister from, DataRegister with) -> string;
                     auto disassembleROXR(EffectiveAddress with) -> string;
                     auto disassembleRTE() -> string;
                     auto disassembleRTR() -> string;
                     auto disassembleRTS() -> string;
                     auto disassembleSBCD(EffectiveAddress with, EffectiveAddress from) -> string;
                     auto disassembleSCC(n4 test, EffectiveAddress to) -> string;
                     auto disassembleSTOP() -> string;
  template<u32 Size> auto disassembleSUB(EffectiveAddress from, DataRegister with) -> string;
  template<u32 Size> auto disassembleSUB(DataRegister from, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleSUBA(EffectiveAddress from, AddressRegister with) -> string;
  template<u32 Size> auto disassembleSUBI(EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleSUBQ(n4 immediate, EffectiveAddress with) -> string;
  template<u32 Size> auto disassembleSUBQ(n4 immediate, AddressRegister with) -> string;
  template<u32 Size> auto disassembleSUBX(EffectiveAddress from, EffectiveAddress with) -> string;
                     auto disassembleSWAP(DataRegister with) -> string;
                     auto disassembleTAS(EffectiveAddress with) -> string;
                     auto disassembleTRAP(n4 vector) -> string;
                     auto disassembleTRAPV() -> string;
  template<u32 Size> auto disassembleTST(EffectiveAddress from) -> string;
                     auto disassembleUNLK(AddressRegister with) -> string;

  template<u32 Size> auto _read(n32 addr) -> n32;
  template<u32 Size = Word> auto _readPC() -> n32;
  auto _readDisplacement(n32 base) -> n32;
  auto _readIndex(n32 base) -> n32;
  auto _dataRegister(DataRegister dr) -> string;
  auto _addressRegister(AddressRegister ar) -> string;
  template<u32 Size> auto _immediate() -> string;
  template<u32 Size> auto _address(EffectiveAddress& ea) -> string;
  template<u32 Size> auto _effectiveAddress(EffectiveAddress& ea) -> string;
  auto _branch(n8 displacement) -> string;
  template<u32 Size> auto _suffix() -> string;
  auto _condition(n4 condition) -> string;

  n32 _pc;
  function<string ()> disassembleTable[65536];
};

}
