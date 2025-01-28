//Peripheral Interface

struct PI : Memory::RCP<PI> {
  Node::Object node;

  struct BBAccess;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto ioBuffers(bool mode, u32 address, u32 data, const char *buffer) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;
    auto ide(bool mode, u2 which, u16 data) -> void;

    struct Memory {
      Node::Debugger::Memory buffer;
    } memory;

    struct Properties {
      Node::Debugger::Properties atb;
    } properties;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
      Node::Debugger::Tracer::Notification ide[4];
    } tracer;
  } debugger;

  struct BBRTC {
    enum State : u8 {
      Address,
      Read,
      Write,
    };

    enum Phase : u8 {
      Sample,
      Setup,
    };

    Memory::Writable ram;

    n2 stored_linestate = 0b11;

    n7 addr = 0;
    n8 data_addr = 0;
    n4 bit_count = 0;
    n4 byte_count = 0;
    b1 read = 0;
    b1 enabled = 0;

    u8 phase;
    u8 state;

    //rtc.cpp
    auto load() -> void;
    auto reset() -> void;
    auto save() -> void;
    auto serialize(serializer& s) -> void;
    auto tick() -> void;
    auto tickClock() -> void;
    auto tickSecond() -> void;
    auto seconds() -> n7;
    auto minutes() -> n7;
    auto hours() -> n6;
    auto day() -> n3;
    auto date() -> n6;
    auto month() -> n5;
    auto years() -> n8;
    auto st() -> n1;
    auto of() -> n1;
    auto ceb() -> n1;
    auto cb() -> n1;
    auto out() -> n1;
    auto set_seconds(n7 value) -> void;
    auto set_minutes(n7 value) -> void;
    auto set_hours(n6 value) -> void;
    auto set_day(n3 value) -> void;
    auto set_date(n6 value) -> void;
    auto set_month(n5 value) -> void;
    auto set_years(n8 value) -> void;
    auto set_st(n1 value) -> void;
    auto set_of(n1 value) -> void;
    auto set_ceb(n1 value) -> void;
    auto set_cb(n1 value) -> void;
    auto set_out(n1 value) -> void;
    auto valid() -> bool;
    auto daysInMonth(u8 month, u8 year) -> u8;
  } bb_rtc;

  //pi.cpp
  auto load(Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;
  auto access() -> BBAccess;

  //dma.cpp
  auto atbMatch(u32 address, u32 i) -> bool;
  auto atbLookup(u32 address) -> u32;
  auto bufferDMARead() -> void;
  auto bufferDMAWrite() -> void;
  auto dmaRead() -> void;
  auto dmaWrite() -> void;
  auto dmaFinished() -> void;
  auto dmaDuration(bool read) -> u32;

  //io.cpp
  auto aesCommandFinished() -> void;
  auto nandCommandFinished() -> void;
  auto regsRead(u32 address) -> u32;
  auto bufRead(u32 address) -> u32;
  auto atbRead(u32 address) -> u32;
  auto ideRead(u32 address) -> u32;
  auto ioRead(u32 address) -> u32;
  auto regsWrite(u32 address, u32 data) -> void;
  auto bufWrite(u32 address, u32 data) -> void;
  auto atbWrite(u32 address, u32 data) -> void;
  auto ideWrite(u32 address, u32 data) -> void;
  auto ioWrite(u32 address, u32 data) -> void;
  auto flushIDE() -> void;

  //bus.hpp
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;
  auto writeFinished() -> void;
  auto writeForceFinish() -> u32;
  template <u32 Size>
  auto busRead(u32 address) -> u32;
  template <u32 Size>
  auto busWrite(u32 address, u32 data) -> void;
  
  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n1  dmaBusy;
    n1  ioBusy;
    n1  error;
    n1  interrupt;
    n32 dramAddress;
    n32 pbusAddress;
    n32 readLength;
    n32 writeLength;
    n32 busLatch;
    u64 originPc = 0;
  } io;

  struct BSD {
    n8 latency;
    n8 pulseWidth;
    n4 pageSize;
    n2 releaseDuration;
  } bsd1, bsd2;

  struct TriState {
    n1 lineOut;
    n1 lineIn;
    n1 outputEnable;
  };

  struct BoxID {
    static constexpr u8 model = 0x01;  //model identifier
    static constexpr u8 clock = 0x02;  //clock speed
    static constexpr u8 unk = 0x01;
  } box_id;

  struct BBGPIO {
    TriState power = { .outputEnable = 1 };
    TriState led = { .outputEnable = 1 };
    TriState rtc_clock = { .lineOut = 1, .lineIn = 1 }; //pullup resistors
    TriState rtc_data = { .lineOut = 1, .lineIn = 1 };
  } bb_gpio;

  struct BBAccess {
    n1 buf;
    n1 flash;
    n1 atb;
    n1 aes;
    n1 dma;
    n1 gpio;
    n1 ide;
    n1 err;
  } bb_allowed;

  struct BBIDE {
    n16 data;
    n1 dirty;
  };

  BBIDE bb_ide[4];

  struct BB_NAND {
    Memory::Writable buffer;

    struct {
      //BB_NAND_CTRL: read
      n1 busy;
      n1 sbErr;
      n1 dbErr;

      //BB_NAND_CTRL: write
      n1 intrDone;
      n6 unk24_29;
      u8 command;
      n1 unk15;
      n1 bufferSel;
      n2 deviceSel;
      n1 ecc;
      n1 multiCycle;
      n10 xferLen;

      n32 config;

      //BB_NAND_ADDR
      n27 pageNumber;
    } io;

    NAND* nand[4];
  } bb_nand;

  struct BB_AES {
    n1 intrPending;
    n1 intrDone;
    n1 busy;
    n1 chainIV;
    n7 bufferOffset;
    n6 dataSize;
    n7 ivOffset;
  } bb_aes;

  struct BB_ATB {
    static constexpr u32 MaxEntries = 192;
    struct {
      n1 ivSource;
      n1 dmaEnable;
      n1 cpuEnable;
      u32 maxOffset;
    } upper;
    struct Entry {
      n1 ivSource;
      n1 dmaEnable;
      n1 cpuEnable;
      u32 maxOffset;
      u32 nandAddr;
    } entries[MaxEntries];
    u32 pbusAddresses[MaxEntries];
    u32 addressMasks[MaxEntries];

    u32 entryCached = -1u;
  } bb_atb;
};

extern PI pi;
