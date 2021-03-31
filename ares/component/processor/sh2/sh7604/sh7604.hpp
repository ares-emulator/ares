#pragma once

//Hitachi SH7604

//struct SH2 {
  enum : u32 { Byte, Word, Long };

  struct Area { enum : u32 {
    Cached   = 0,
    Uncached = 1,
    Purge    = 2,
    Address  = 3,
    Data     = 6,
    IO       = 7,
  };};

  //bus.cpp
  auto readByte(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto readLong(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeLong(u32 address, u32 data) -> void;

  //io.cpp
  auto internalReadByte(u32 address, n8 data = 0) -> n8;
  auto internalWriteByte(u32 address, n8 data) -> void;

  struct Cache {
    maybe<SH2&> self;

    //cache.cpp
    template<u32 Size> auto read(u32 address) -> u32;
    template<u32 Size> auto write(u32 address, u32 data) -> void;
    template<u32 Size> auto readData(u32 address) -> u32;
    template<u32 Size> auto writeData(u32 address, u32 data) -> void;
    auto readAddress(u32 address) -> u32;
    auto writeAddress(u32 address, u32 data) -> void;
    auto purge(u32 address) -> void;
    template<u32 Ways> auto purge() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    enum : u32 {
      Way0 = 0 * 64,
      Way1 = 1 * 64,
      Way2 = 2 * 64,
      Way3 = 3 * 64,
      Invalid = 1 << 19,
    };

    union Line {
      u8  bytes[16];
      u16 words[8];
      u32 longs[4];
    };

    u8   lrus[64];
    u32  tags[4 * 64];
    Line lines[4 * 64];

    n1 enable;
    n1 disableCode;
    n1 disableData;
    n2 twoWay;  //0 = 4-way, 2 = 2-way (forces ways 2 and 3)
    n2 waySelect;

  //internal:
    n2 lruSelect[64];
    n6 lruUpdate[4][64];
  } cache;

  //interrupt controller
  struct INTC {
    maybe<SH2&> self;

    //interrupts.cpp
    auto run() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct ICR {    //interrupt control register
      n1 vecmd;     //IRL interrupt vector mode select
      n1 nmie;      //NMI edge select
      n1 nmil;      //NMI input level
    } icr;
    struct IPRA {   //interrupt priority level setting register A
      n4 wdtip;     //WDT interrupt priority
      n4 dmacip;    //DMAC interrupt priority
      n4 divuip;    //DIVU interrupt priority
    } ipra;
    struct IPRB {   //interrupt priority level setting register B
      n4 frtip;     //FRT interrupt priority
      n4 sciip;     //SCI interrupt priority
    } iprb;
    struct VCRA {   //vector number setting register A
      n7 srxv;      //SCI receive data full interrupt vector number
      n7 serv;      //SCI receive error interrupt vector number
    } vcra;
    struct VCRB {   //vector number setting register B
      n7 stev;      //SCI transmit end interrupt vector number
      n7 stxv;      //SCI transmit data empty interrupt vector number
    } vcrb;
    struct VCRC {   //vector number setting register C
      n7 focv;      //FRT output compare interrupt vector number
      n7 ficv;      //FRT input capture interrupt vector number
    } vcrc;
    struct VCRD {   //vector number setting register D
      n7 fovv;      //FRT overflow interrupt vector number
    } vcrd;
    struct VCRWDT { //vector number setting register WDT
      n4 bcmv;      //BSC compare match interrupt vector number
      n4 witv;      //WDT interval interrupt vector number
    } vcrwdt;
  } intc;

  //DMA controller
  struct DMAC {
    maybe<SH2&> self;

    //dma.cpp
    auto run() -> void;
    auto transfer(bool c) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n32 sar[2];     //DMA source address registers
    n32 dar[2];     //DMA destination address registers
    n24 tcr[2];     //DMA transfer count registers
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
    } chcr[2];
    n8  vcrdma[2];  //DMA vector number registers
    n2  drcr[2];    //DMA request/response selection control registers
    struct DMAOR {  //DMA operation register
      n1 dme;       //DMA master enable bit
      n1 nmif;      //NMI flag bit
      n1 ae;        //address error flag bit
      n1 pr;        //priority mode bit
    } dmaor;

    //internal:
    n1 dreq;
    n2 pendingIRQ;
  } dmac;

  //serial communication interface
  struct SCI {
    maybe<SH2&> self;
    maybe<SH2&> link;

    //serial.cpp
    auto run() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct SMR {    //serial mode register
      n2 cks;       //clock select
      n1 mp;        //multi-processor mode
      n1 stop;      //stop bit length
      n1 oe;        //parity mode
      n1 pe;        //parity enable
      n1 chr;       //character length
      n1 ca;        //communication mode
    } smr;
    struct SCR {    //serial control register
      n2 cke;       //clock enable
      n1 teie;      //transmit end interrupt enable
      n1 mpie;      //multi-processor interrupt enable
      n1 re;        //receive enable
      n1 te;        //transmit enable
      n1 rie;       //receive interrupt enable
      n1 tie;       //transmit interrupt enable
    } scr;
    struct SSR {    //serial status register
      n1 mpbt;      //multi-processor bit transfer
      n1 mpb;       //multi-processor bit
      n1 tend = 1;  //transmit end
      n1 per;       //parity error
      n1 fer;       //framing error
      n1 orer;      //overrun error
      n1 rdrf;      //receive data register full
      n1 tdre;      //transmit data register empty
    } ssr;
    n8 brr = 0xff;  //bit rate register
    n8 tdr = 0xff;  //transmit data register
    n8 rdr = 0x00;  //receive data register

    //internal:
    n1 pendingTransmitEmptyIRQ;
    n1 pendingReceiveFullIRQ;
  } sci;

  //watchdog timer
  struct WDT {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct WTCSR {  //watchdog timer control/status register
      n3 cks;       //clock select
      n1 tme;       //timer enable
      n1 wtit;      //timer mode select
      n1 ovf;       //overflow flag
    } wtcsr;
    struct RSTCSR { //reset control/status register
      n1 rsts;      //reset select
      n1 rste;      //reset enable
      n1 wovf;      //watchdog timer overflow flag
    } rstcsr;
    n8 wtcnt;       //watchdog timer counter
  } wdt;

  //user break controller
  struct UBC {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n32 bara;       //break address register A
    n32 bamra;      //break address mask register A
    struct BBRA {   //break bus register A
      n2 sza;       //operand size select A
      n2 rwa;       //read/write select A
      n2 ida;       //instruction fetch/data access select A
      n2 cpa;       //CPU cycle/peripheral cycle select A
    } bbra;
    n32 barb;       //break address register B
    n32 bamrb;      //break address mask register B
    struct BBRB {   //break bus register B
      n2 szb;       //operand size select B
      n2 rwb;       //read/write select B
      n2 idb;       //instruction fetch/data access select B
      n2 cpb;       //CPU cycle/peripheral cycle select B
    } bbrb;
    n32 bdrb;       //break data register B
    n32 bdmrb;      //break data mask register B
    struct BRCR {   //break control register
      n1 pcbb;      //instruction break select B
      n1 dbeb;      //data break enable B
      n1 seq;       //sequence condition select
      n1 cmfpb;     //peripheral condition match flag B
      n1 cmfcb;     //CPU condition match flag B
      n1 pcba;      //PC break select A
      n1 umd;       //UBC mode
      n1 ebbe;      //external bus break enable
      n1 cmfpa;     //peripheral condition match flag A
      n1 cmfca;     //CPU condition match flag A
    } brcr;
  } ubc;

  //16-bit free-running timer
  struct FRT {
    maybe<SH2&> self;

    //timer.cpp
    auto run() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct TIER {   //timer interrupt enable register
      n1 ovie;      //timer overflow interrupt enable
      n1 ociae;     //output compare interrupt enable A
      n1 ocibe;     //output compare interrupt enable B
      n1 icie;      //input capture interrupt enable
    } tier;
    struct FTCSR {  //free-running timer control/status register
      n1 cclra;     //counter clear A
      n1 ovf;       //timer overflow flag
      n1 ocfa;      //output compare flag A
      n1 ocfb;      //output compare flag B
      n1 icf;       //input capture flag
    } ftcsr;
    n16 frc;        //free-running counter
    n16 ocra;       //output compare register A
    n16 ocrb;       //output compare register B
    struct TCR {    //timer control register
      n2 cks;       //clock select
      n1 iedg;      //input edge select
    } tcr;
    struct TOCR {   //timer output compare control register
      n1 olvla;     //output level A
      n1 olvlb;     //output level B
      n1 ocrs;      //output compare register select
    } tocr;
    n16 ficr;       //input capture register

    //internal:
    n32 counter;
    n1  pendingOutputIRQ;
  } frt;

  //bus state controller
  struct BSC {
    //serialization.cpp
    auto serialize(serializer&) -> void;

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

  //standby control register
  struct SBYCR {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 sci;         //SCI halted
    n1 frt;         //FRT halted
    n1 divu;        //DIVU halted
    n1 mult;        //MULT halted
    n1 dmac;        //DMAC halted
    n1 hiz;         //port high impedance
    n1 sby;         //standby
  } sbycr;

  //division unit
  struct DIVU {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n32 dvsr;       //divisor register
    struct DVCR {   //division control register
      n1 ovf;       //overflow flag
      n1 ovfie;     //overflow interrupt enable
    } dvcr;
    n7 vcrdiv;      //vector number setting register
    n32 dvdnth;     //dividend register H
    n32 dvdntl;     //dividend register L
  } divu;
//};
