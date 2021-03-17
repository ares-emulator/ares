#pragma once

//Hitachi SH-2 (SH7604)

namespace ares {

struct SH2 {
  virtual auto exception() -> bool = 0;
  virtual auto step(u32 clocks) -> void = 0;
  virtual auto busReadByte(u32 address) -> u32 = 0;
  virtual auto busReadWord(u32 address) -> u32 = 0;
  virtual auto busReadLong(u32 address) -> u32 = 0;
  virtual auto busWriteByte(u32 address, u32 data) -> void = 0;
  virtual auto busWriteWord(u32 address, u32 data) -> void = 0;
  virtual auto busWriteLong(u32 address, u32 data) -> void = 0;

  //sh2.cpp
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //memory.cpp
  auto readByte(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto readLong(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeLong(u32 address, u32 data) -> void;
  auto internalReadByte(u32 address, n8 data = 0) -> n8;
  auto internalWriteByte(u32 address, n8 data) -> void;

  //instruction.cpp
  auto branch(u32 pc) -> void;
  auto delaySlot(u32 pc) -> void;
  auto interrupt(u32 level, u32 vector) -> void;
  auto instruction() -> void;
  auto execute(u16 opcode) -> void;

  //instructions.cpp
  inline auto ADD(u32 m, u32 n) -> void;
  inline auto ADDC(u32 m, u32 n) -> void;
  inline auto ADDI(u32 i, u32 n) -> void;
  inline auto ADDV(u32 m, u32 n) -> void;
  inline auto AND(u32 m, u32 n) -> void;
  inline auto ANDI(u32 i) -> void;
  inline auto ANDM(u32 i) -> void;
  inline auto BF(u32 d) -> void;
  inline auto BFS(u32 d) -> void;
  inline auto BRA(u32 d) -> void;
  inline auto BRAF(u32 m) -> void;
  inline auto BSR(u32 d) -> void;
  inline auto BSRF(u32 m) -> void;
  inline auto BT(u32 d) -> void;
  inline auto BTS(u32 d) -> void;
  inline auto CLRMAC() -> void;
  inline auto CLRT() -> void;
  inline auto CMPEQ(u32 m, u32 n) -> void;
  inline auto CMPGE(u32 m, u32 n) -> void;
  inline auto CMPGT(u32 m, u32 n) -> void;
  inline auto CMPHI(u32 m, u32 n) -> void;
  inline auto CMPHS(u32 m, u32 n) -> void;
  inline auto CMPIM(u32 i) -> void;
  inline auto CMPPL(u32 n) -> void;
  inline auto CMPPZ(u32 n) -> void;
  inline auto CMPSTR(u32 m, u32 n) -> void;
  inline auto DIV0S(u32 m, u32 n) -> void;
  inline auto DIV0U() -> void;
  inline auto DIV1(u32 m, u32 n) -> void;
  inline auto DMULS(u32 m, u32 n) -> void;
  inline auto DMULU(u32 m, u32 n) -> void;
  inline auto DT(u32 n) -> void;
  inline auto EXTSB(u32 m, u32 n) -> void;
  inline auto EXTSW(u32 m, u32 n) -> void;
  inline auto EXTUB(u32 m, u32 n) -> void;
  inline auto EXTUW(u32 m, u32 n) -> void;
  inline auto JMP(u32 m) -> void;
  inline auto JSR(u32 m) -> void;
  inline auto LDCSR(u32 m) -> void;
  inline auto LDCGBR(u32 m) -> void;
  inline auto LDCVBR(u32 m) -> void;
  inline auto LDCMSR(u32 m) -> void;
  inline auto LDCMGBR(u32 m) -> void;
  inline auto LDCMVBR(u32 m) -> void;
  inline auto LDSMACH(u32 m) -> void;
  inline auto LDSMACL(u32 m) -> void;
  inline auto LDSPR(u32 m) -> void;
  inline auto LDSMMACH(u32 m) -> void;
  inline auto LDSMMACL(u32 m) -> void;
  inline auto LDSMPR(u32 m) -> void;
  inline auto MACW(u32 m, u32 n) -> void;
  inline auto MACL_(u32 m, u32 n) -> void;
  inline auto MOV(u32 m, u32 n) -> void;
  inline auto MOVBL(u32 m, u32 n) -> void;
  inline auto MOVBL0(u32 m, u32 n) -> void;
  inline auto MOVBL4(u32 m, u32 d) -> void;
  inline auto MOVBLG(u32 d) -> void;
  inline auto MOVBM(u32 m, u32 n) -> void;
  inline auto MOVBP(u32 m, u32 n) -> void;
  inline auto MOVBS(u32 m, u32 n) -> void;
  inline auto MOVBS0(u32 m, u32 n) -> void;
  inline auto MOVBS4(u32 d, u32 n) -> void;
  inline auto MOVBSG(u32 d) -> void;
  inline auto MOVWL(u32 m, u32 n) -> void;
  inline auto MOVWL0(u32 m, u32 n) -> void;
  inline auto MOVWL4(u32 m, u32 d) -> void;
  inline auto MOVWLG(u32 d) -> void;
  inline auto MOVWM(u32 m, u32 n) -> void;
  inline auto MOVWP(u32 m, u32 n) -> void;
  inline auto MOVWS(u32 m, u32 n) -> void;
  inline auto MOVWS0(u32 m, u32 n) -> void;
  inline auto MOVWS4(u32 d, u32 n) -> void;
  inline auto MOVWSG(u32 d) -> void;
  inline auto MOVLL(u32 m, u32 n) -> void;
  inline auto MOVLL0(u32 m, u32 n) -> void;
  inline auto MOVLL4(u32 m, u32 d, u32 n) -> void;
  inline auto MOVLLG(u32 d) -> void;
  inline auto MOVLM(u32 m, u32 n) -> void;
  inline auto MOVLP(u32 m, u32 n) -> void;
  inline auto MOVLS(u32 m, u32 n) -> void;
  inline auto MOVLS0(u32 m, u32 n) -> void;
  inline auto MOVLS4(u32 m, u32 d, u32 n) -> void;
  inline auto MOVLSG(u32 d) -> void;
  inline auto MOVI(u32 i, u32 n) -> void;
  inline auto MOVWI(u32 d, u32 n) -> void;
  inline auto MOVLI(u32 d, u32 n) -> void;
  inline auto MOVA(u32 d) -> void;
  inline auto MOVT(u32 n) -> void;
  inline auto MULL(u32 m, u32 n) -> void;
  inline auto MULS(u32 m, u32 n) -> void;
  inline auto MULU(u32 m, u32 n) -> void;
  inline auto NEG(u32 m, u32 n) -> void;
  inline auto NEGC(u32 m, u32 n) -> void;
  inline auto NOP() -> void;
  inline auto NOT(u32 m, u32 n) -> void;
  inline auto OR(u32 m, u32 n) -> void;
  inline auto ORI(u32 i) -> void;
  inline auto ORM(u32 i) -> void;
  inline auto ROTCL(u32 n) -> void;
  inline auto ROTCR(u32 n) -> void;
  inline auto ROTL(u32 n) -> void;
  inline auto ROTR(u32 n) -> void;
  inline auto RTE() -> void;
  inline auto RTS() -> void;
  inline auto SETT() -> void;
  inline auto SHAL(u32 n) -> void;
  inline auto SHAR(u32 n) -> void;
  inline auto SHLL(u32 n) -> void;
  inline auto SHLL2(u32 n) -> void;
  inline auto SHLL8(u32 n) -> void;
  inline auto SHLL16(u32 n) -> void;
  inline auto SHLR(u32 n) -> void;
  inline auto SHLR2(u32 n) -> void;
  inline auto SHLR8(u32 n) -> void;
  inline auto SHLR16(u32 n) -> void;
  inline auto SLEEP() -> void;
  inline auto STCSR(u32 n) -> void;
  inline auto STCGBR(u32 n) -> void;
  inline auto STCVBR(u32 n) -> void;
  inline auto STCMSR(u32 n) -> void;
  inline auto STCMGBR(u32 n) -> void;
  inline auto STCMVBR(u32 n) -> void;
  inline auto STSMACH(u32 n) -> void;
  inline auto STSMACL(u32 n) -> void;
  inline auto STSPR(u32 n) -> void;
  inline auto STSMMACH(u32 n) -> void;
  inline auto STSMMACL(u32 n) -> void;
  inline auto STSMPR(u32 n) -> void;
  inline auto SUB(u32 m, u32 n) -> void;
  inline auto SUBC(u32 m, u32 n) -> void;
  inline auto SUBV(u32 m, u32 n) -> void;
  inline auto SWAPB(u32 m, u32 n) -> void;
  inline auto SWAPW(u32 m, u32 n) -> void;
  inline auto TAS(u32 n) -> void;
  inline auto TRAPA(u32 i) -> void;
  inline auto TST(u32 m, u32 n) -> void;
  inline auto TSTI(u32 i) -> void;
  inline auto TSTM(u32 i) -> void;
  inline auto XOR(u32 m, u32 n) -> void;
  inline auto XORI(u32 i) -> void;
  inline auto XORM(u32 i) -> void;
  inline auto XTRCT(u32 m, u32 n) -> void;

  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;

  static constexpr u32 undefined = 0;

  struct Branch {
    enum : u32 { Step, Slot, Take };
  };

  struct S32 {
    operator u32() const {
      return T << 0 | S << 1 | I << 4 | Q << 8 | M << 9;
    }

    auto& operator=(u32 d) {
      T = d >> 0 & 1;
      S = d >> 1 & 1;
      I = d >> 4 & 15;
      Q = d >> 8 & 1;
      M = d >> 9 & 1;
      return *this;
    }

    n1 T;
    n1 S;
    n4 I;
    n1 Q;
    n1 M;
  };

  u32 R[16];  //general purpose registers
  S32 SR;     //status register
  u32 GBR;    //global base register
  u32 VBR;    //vector base register
  union {
    u64 MAC;  //multiply-and-accumulate register
    struct { u32 order_msb2(MACH, MACL); };
  };
  u32 PR;     //procedure register
  u32 PC;     //program counter
  u32 PPC;    //program counter for delay slots
  u32 PPM;    //delay slot mode

  u8 cache[4_KiB];

  //DMA controller
  struct DMAC {
    n32 sar;        //DMA source address registers
    n32 dar;        //DMA destination address registers
    n24 tcr;        //DMA transfer count registers
    struct CHCR {   //DMA channel control registers
      n1 de;        //DMA enable
      n1 te;        //transfer end flag
      n1 ie;        //interrupt enable
      n1 ta;        //transfer address mode
      n1 tb;        //transfer bus mode
      n1 dl;        //DREQn level bit
      n1 ds;        //DREQn select bit
      n1 al;        //acknowledge level bit
      n1 am;        //acknowledge/transfer mode bit
      n1 ar;        //auto request mode bit
      n2 ts;        //transfer size
      n2 sm;        //source address mode
      n2 dm;        //destination address mode
    } chcr;
    n8  vcrdma;     //DMA vector number registers
    n2  drcr;       //DMA request/response selection control registers
  } dmac[2];
  struct DMAOR {
    n1 dme;         //DMA master enable bit
    n1 nmif;        //NMI flag bit
    n1 ae;          //address error flag bit
    n1 pr;          //priority mode bit
  } dmaor;

  //16-bit free-running timer
  struct FRT {
    struct TIER {   //timer interrupt enable register
      n1 ovie;      //timer overflow interrupt enable
      n1 ocie[2];   //output compare interrupt enables
      n1 icie;      //input capture interrupt enable
    } tier;
    struct FTCSR {  //free-running timer control/status register
      n1 cclra;     //counter clear A
      n1 ovf;       //timer overflow flag
      n1 ocf[2];    //output compare flags
      n1 icf;       //input capture flag
    } ftcsr;
    n16 frc;        //free-running counter
    n16 ocr[2];     //output compare registers
    struct TCR {    //timer control register
      n2 cks;       //clock select
      n1 iedg;      //input edge select
    } tcr;
    struct TOCR {   //timer output compare control register
      n1 olvl[2];   //output levels
      n1 ocrs;      //output compare register select
    } tocr;
    n16 ficr;       //input capture register
  } frt;

  //bus state controller
  struct BSC {
    struct BCR1 {   //bus control register 1
      n3 dram;      //enable for DRAM and other memory
      n2 a0lw;      //long wait specification for area 0
      n2 a1lw;      //long wait specification for area 1
      n2 ahlw;      //long wait specification for ares 2 and 3
      n1 pshr;      //partial space share specification
      n1 bstrom;    //area 0 burst ROM enable
      n1 a2endian;  //endian specification for area 2
      n1 master;    //bus arbitration (0 = master; 1 = slave)
    } bcr1;
    struct BCR2 {   //bus control register 2
      n2 a1sz;      //bus size specification for area 1
      n2 a2sz;      //bus size specification for area 2
      n2 a3sz;      //bus size specification for area 3
    } bcr2;
    struct WCR {    //wait control register
      n2 w0;        //wait control for area 0
      n2 w1;        //wait control for area 1
      n2 w2;        //wait control for area 2
      n2 w3;        //wait control for area 3
      n2 iw0;       //idles between cycles for area 0
      n2 iw1;       //idles between cycles for area 1
      n2 iw2;       //idles between cycles for area 2
      n2 iw3;       //idles between cycles for area 3
    } wcr;
    struct MCR {    //individual memory control register
      n1 rcd;       //RAS-CAS delay
      n1 trp;       //RSA precharge time
      n1 rmode;     //refresh mode
      n1 rfsh;      //refresh control
      n3 amx;       //address multiplex
      n1 sz;        //memory data size
      n1 trwl;      //write-precharge delay
      n1 rasd;      //bank active mode
      n1 be;        //burst enable
      n2 tras;      //CAS before RAS refresh RAS assert time
    } mcr;
    struct RTCSR {  //refresh timer control/status register
      n3 cks;       //clock select
      n1 cmie;      //compare match interrupt enable
      n1 cmf;       //compare match flag
    } rtcsr;
    n8 rtcnt;       //refresh timer counter
    n8 rtcor;       //refresh time constant register
  } bsc;

  struct CCR {      //cache control register
    n1 ce;          //cache enable
    n1 id;          //instruction replacement disable
    n1 od;          //data replacement disable
    n1 tw;          //two-way mode
    n1 cp;          //cache purge
    n2 w;           //way specification
  } ccr;

  struct SBYCR {    //standby control register
    n1 sci;         //SCI halted
    n1 frt;         //FRT halted
    n1 divu;        //DIVU halted
    n1 mult;        //MULT halted
    n1 dmac;        //DMAC halted
    n1 hiz;         //port high impedance
    n1 sby;         //standby
  } sbycr;

  struct DIVU {     //division unit
    n32 dvsr;       //divisor register
    struct DVCR {   //division control register
      n1 ovf;       //overflow flag
      n1 ovfie;     //overflow interrupt enable
    } dvcr;
    n7 vcrdiv;      //vector number setting register
    n32 dvdnth;     //dividend register H
    n32 dvdntl;     //dividend register L
  } divu;
};

}
