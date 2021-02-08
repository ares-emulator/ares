//Hudson Soft HuC6270: Video Display Controller

struct VDC {
  struct Debugger {
    //debugger.cpp
    auto load(VDC&, Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory satb;
    } memory;
  } debugger;

  auto burstMode() const -> bool { return latch.burstMode || timing.vstate != VDW; }
  auto irqLine() const -> bool { return irq.line; }

  //vdc.cpp
  auto hsync() -> void;
  auto vsync() -> void;
  auto hclock() -> void;
  auto vclock() -> void;
  auto read(n2 address) -> n8;
  auto write(n2 address, n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n9 output[512];

  struct VRAM {
    //vdc.cpp
    auto read(n16 address) const -> n16;
    auto write(n16 address, n16 data) -> void;

    n16 memory[0x8000];

    n16 addressRead;
    n16 addressWrite;
    n16 addressIncrement = 0x01;

    n16 dataRead;
    n16 dataWrite;
  } vram;

  struct SATB {
    //vdc.cpp
    auto read(n8 address) const -> n16;
    auto write(n8 address, n16 data) -> void;

    n16 memory[0x100];
  } satb;

  struct IRQ {
    enum class Line : u32 {
      Collision,
      Overflow,
      Coincidence,
      Vblank,
      TransferVRAM,
      TransferSATB,
    };

    struct Source {
      n1 enable;
      n1 pending;
    };

    //irq.cpp
    auto poll() -> void;
    auto raise(Line) -> void;
    auto lower() -> void;

    n1 line;

    Source collision;
    Source overflow;
    Source coincidence;
    Source vblank;
    Source transferVRAM;
    Source transferSATB;
  } irq;

  struct DMA {
    maybe<VDC&> vdc;

    //dma.cpp
    auto step(u32 clocks) -> void;
    auto vramStart() -> void;
    auto satbStart() -> void;
    auto satbQueue() -> void;

    n1  sourceIncrementMode;
    n1  targetIncrementMode;
    n1  satbRepeat;
    n16 source;
    n16 target;
    n16 length;
    n16 satbSource;

    n1  vramActive;
    n1  satbActive;
    n1  satbPending;
    n8  satbOffset;
  } dma;

  enum : u32 { HDS, HDW, HDE, HSW };
  enum : u32 { VSW, VDS, VDW, VCR };

  struct Timing {
    n5  horizontalSyncWidth = 2;
    n7  horizontalDisplayStart = 2;
    n7  horizontalDisplayWidth = 31;
    n7  horizontalDisplayEnd = 4;

    n5  verticalSyncWidth = 2;
    n8  verticalDisplayStart = 15;
    n9  verticalDisplayWidth = 239;
    n8  verticalDisplayEnd = 4;

    n8  hstate = HDS;
    n8  vstate = VSW;

    n16 hoffset;
    n16 voffset;

    n10 coincidence = 64;
  } timing;

  struct Latch {
    n16 horizontalSyncWidth;
    n16 horizontalDisplayStart;
    n16 horizontalDisplayWidth;
    n16 horizontalDisplayEnd;

    n16 verticalSyncWidth;
    n16 verticalDisplayStart;
    n16 verticalDisplayWidth;
    n16 verticalDisplayEnd;

    n1  burstMode = 1;
  } latch;

  struct IO {
    n5  address;

    n2  externalSync;
    n2  displayOutput;
    n1  dramRefresh;
    n10 coincidence;
  } io;

  struct Background {
    maybe<VDC&> vdc;

    //background.cpp
    auto scanline(n16 y) -> void;
    auto render(n16 y) -> void;

    n1  enable;
    n2  vramMode;  //partially emulated
    n1  characterMode;
    n10 hscroll;
    n9  vscroll;
    n9  vcounter;
    n8  width = 32;
    n8  height = 32;

    n10 hoffset;
    n9  voffset;

    struct Latch {
      n2 vramMode;
      n1 characterMode;
    } latch;

    struct Output {
      n4 color;
      n4 palette;
    } output[512];
  } background;

  struct Object {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n10 y;
    n10 x;
    n1  characterMode;
    n10 pattern;
    n4  palette;
    n1  priority;
    n8  width;
    n8  height;
    n1  hflip;
    n1  vflip;
    n1  first;
  };

  struct Sprite {
    maybe<VDC&> vdc;

    //sprite.cpp
    auto scanline(n16 y) -> void;
    auto render(n16 y) -> void;

    adaptive_array<Object, 16> objects;

    n1 enable;
    n2 vramMode;  //partially emulated

    struct Latch {
      n2 vramMode;
    } latch;

    struct Output {
      n4 color;
      n4 palette;
      n1 priority;
    } output[512];
  } sprite;
};
