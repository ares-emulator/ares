//Mega 32X

struct M32X {
  Node::Object node;
  Memory::Readable<n16> rom;
  Memory::Readable<n16> vectors;
  Memory::Writable<n16> sdram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory sdram;
    } memory;
  } debugger;

  struct SH7604 : SH2, Thread {
    Node::Object node;
    Memory::Readable<n16> bootROM;

    struct Debugger {
      maybe<SH7604&> self;

      //debugger.cpp
      auto load(Node::Object) -> void;
      auto instruction() -> void;
      auto interrupt(string_view) -> void;

      struct Tracer {
        Node::Debugger::Tracer::Instruction instruction;
        Node::Debugger::Tracer::Notification interrupt;
      } tracer;
    } debugger;

    //sh.cpp
    auto load(Node::Object, string name, string bootROM) -> void;
    auto unload() -> void;

    auto main() -> void;
    auto step(u32 clocks) -> void override;
    auto power(bool reset) -> void;

    auto busReadByte(u32 address) -> u32 override;
    auto busReadWord(u32 address) -> u32 override;
    auto busReadLong(u32 address) -> u32 override;
    auto busWriteByte(u32 address, u32 data) -> void override;
    auto busWriteWord(u32 address, u32 data) -> void override;
    auto busWriteLong(u32 address, u32 data) -> void override;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IRQ {
      struct Source {
        n1 enable;
        n1 active;
      };
      Source pwm;   // 6
      Source cmd;   // 8
      Source hint;  //10
      Source vint;  //12
      Source vres;  //14
    } irq;
  };

  struct VDP {
    maybe<M32X&> self;
    Memory::Writable<n16> dram;
    Memory::Writable<n16> cram;
    array_span<n16> fbram;  //VDP-side active DRAM bank
    array_span<n16> bbram;  //CPU-side active DRAM bank

    struct Debugger {
      //debugger.cpp
      auto load(Node::Object) -> void;

      struct Memory {
        Node::Debugger::Memory dram;
        Node::Debugger::Memory cram;
      } memory;
    } debugger;

    //vdp.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto power(bool reset) -> void;

    auto scanline(u32 pixels[1280], u32 y) -> void;
    auto scanlineMode1(u32 pixels[1280], u32 y) -> void;
    auto scanlineMode2(u32 pixels[1280], u32 y) -> void;
    auto scanlineMode3(u32 pixels[1280], u32 y) -> void;
    auto plot(u32* output, u16 color) -> void;
    auto fill() -> void;
    auto selectFramebuffer(n1 active) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n2  mode;      //0 = blank, 1 = packed, 2 = direct, 3 = RLE
    n1  lines;     //0 = 224, 1 = 240
    n1  priority;  //0 = MD, 1 = 32X
    n1  dotshift;
    n8  autofillLength;
    n16 autofillAddress;
    n16 autofillData;
    n1  framebufferAccess;
    n1  framebufferActive;
    n1  framebufferSelect;
    n1  hblank;
    n1  vblank;
  };

  struct PWM : Thread {
    Node::Audio::Stream stream;

    //pwm.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto main() -> void;
    auto step(u32 clocks) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n2  lmode;
    n2  rmode;
    n1  mono;
    n1  dreqIRQ;
    n4  timer;
    n12 cycle;

    n32 periods;
    n32 counter;
    i12 lsample;
    i12 rsample;
    queue<i12[4096]> lfifo;
    queue<i12[4096]> rfifo;
  };

  //m32x.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto save() -> void;

  auto power(bool reset) -> void;

  auto vblank(bool) -> void;
  auto hblank(bool) -> void;

  //bus-internal.cpp
  auto readInternal(n1 upper, n1 lower, n32 address, n16 data = 0) -> n16;
  auto writeInternal(n1 upper, n1 lower, n32 address, n16 data) -> void;

  //bus-external.cpp
  auto readExternal(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeExternal(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //io-internal.cpp
  auto readInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> n16;
  auto writeInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> void;

  //io-external.cpp
  auto readExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  SH7604 shm;  //master
  SH7604 shs;  //slave
  VDP vdp;
  PWM pwm;

  struct IO {
    //$000070
    n32 vectorLevel4;

    //$a15000
    n1 adapterEnable;
    n1 adapterReset;
    n1 resetEnable = 1;

    //$a15004
    n2 romBank;

    //$a1511a
    n1 cartridgeMode;

    //$4004
    n8 hperiod;
    n8 hcounter;

    n1 hintVblank;
  } io;

  struct DREQ {
    n1 vram;
    n1 dma;
    n1 active;

    n24 source;
    n24 target;
    n16 length;

    queue<n16[8]> fifo;
  } dreq;

  n16 communication[8];
};

extern M32X m32x;
