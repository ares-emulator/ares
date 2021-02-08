//Yamaha YM7101
#if defined(PROFILE_PERFORMANCE)
#include "../vdp-performance/vdp.hpp"
#else
struct VDP : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean overscan;

  struct Debugger {
    VDP& self;
    Debugger(VDP& self) : self(self) {}

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory vsram;
      Node::Debugger::Memory cram;
    } memory;
  } debugger{*this};

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto power(bool reset) -> void;

  //io.cpp
  auto read(n24 address, n16 data) -> n16;
  auto write(n24 address, n16 data) -> void;

  auto readDataPort() -> n16;
  auto writeDataPort(n16 data) -> void;

  auto readControlPort() -> n16;
  auto writeControlPort(n16 data) -> void;

  struct DMA {
    //dma.cpp
    auto run() -> void;
    auto load() -> void;
    auto fill() -> void;
    auto copy() -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 active;

    struct IO {
      n2  mode;
      n22 source;
      n16 length;
      n8  fill;
      n1  enable;
      n1  wait;
    } io;
  } dma;

  //render.cpp
  auto scanline() -> void;
  auto run() -> void;
  auto outputPixel(n32 color) -> void;

  struct Pixel {
    auto above() const -> bool { return priority == 1 && color; }
    auto below() const -> bool { return priority == 0 && color; }

    n6 color;
    n1 priority;
  };

  struct Background {
    enum class ID : u32 { PlaneA, Window, PlaneB } id;

    //background.cpp
    auto isWindowed(u32 x, u32 y) -> bool;

    auto updateHorizontalScroll(u32 y) -> void;
    auto updateVerticalScroll(u32 x) -> void;

    auto nametableAddress() -> n15;
    auto nametableWidth() -> u32;
    auto nametableHeight() -> u32;

    auto scanline(u32 y) -> void;
    auto run(u32 x, u32 y) -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n16 generatorAddress;
      n16 nametableAddress;

      //PlaneA, PlaneB
      n2  nametableWidth;
      n2  nametableHeight;
      n15 horizontalScrollAddress;
      n2  horizontalScrollMode;
      n1  verticalScrollMode;

      //Window
      n10 horizontalOffset;
      n1  horizontalDirection;
      n10 verticalOffset;
      n1  verticalDirection;
    } io;

    struct State {
      n10 horizontalScroll;
      n10 verticalScroll;
    } state;

    Pixel output;
  };
  Background planeA{Background::ID::PlaneA};
  Background window{Background::ID::Window};
  Background planeB{Background::ID::PlaneB};

  struct Object {
    //sprite.cpp
    auto width() const -> u32;
    auto height() const -> u32;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n9  x;
    n10 y;
    n2  tileWidth;
    n2  tileHeight;
    n1  horizontalFlip;
    n1  verticalFlip;
    n2  palette;
    n1  priority;
    n11 address;
    n7  link;
  };

  struct Sprite {
    VDP& vdp;

    //the per-scanline sprite limits are different between H40 and H32 modes
    auto objectLimit() const -> u32 { return vdp.io.displayWidth ? 20 : 16; }
    auto tileLimit()   const -> u32 { return vdp.io.displayWidth ? 40 : 32; }
    auto linkLimit()   const -> u32 { return vdp.io.displayWidth ? 80 : 64; }

    //sprite.cpp
    auto write(n9 address, n16 data) -> void;
    auto scanline(u32 y) -> void;
    auto run(u32 x, u32 y) -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n16 generatorAddress;
      n16 nametableAddress;
    } io;

    Pixel output;

    adaptive_array<Object, 80> oam;
    adaptive_array<Object, 20> objects;
  };
  Sprite sprite{*this};

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  auto pixelWidth() const -> u32 { return latch.displayWidth ? 4 : 5; }
  auto screenWidth() const -> u32 { return latch.displayWidth ? 320 : 256; }
  auto screenHeight() const -> u32 { return latch.overscan ? 240 : 224; }
  auto frameHeight() const -> u32 { return Region::PAL() ? 312 : 262; }

  //video RAM
  struct VRAM {
    //memory.cpp
    auto read(n16 address) const -> n16;
    auto write(n16 address, n16 data) -> void;

    auto readByte(n17 address) const -> n8;
    auto writeByte(n17 address, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    //Mega Drive: 65536x4-bit (x2) =  64KB VRAM
    //Tera Drive: 65536x4-bit (x4) = 128KB VRAM
    n16 memory[65536];  //stored in 16-bit words
    n32 size = 32768;
    n1  mode;  //0 = 64KB, 1 = 128KB
  } vram;

  //vertical scroll RAM
  struct VSRAM {
    //memory.cpp
    auto read(n6 address) const -> n10;
    auto write(n6 address, n10 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n10 memory[40];
  } vsram;

  //color RAM
  struct CRAM {
    //memory.cpp
    auto read(n6 address) const -> n9;
    auto write(n6 address, n9 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n9 memory[64];
  } cram;

  struct IO {
    //status
    n1  vblankIRQ;  //true after VIRQ triggers; cleared at start of next frame

    //command
    n6  command;
    n17 address;
    n1  commandPending;

    //$00  mode register 1
    n1  displayOverlayEnable;
    n1  counterLatch;
    n1  horizontalBlankInterruptEnable;
    n1  leftColumnBlank;

    //$01  mode register 2
    n1  videoMode;  //0 = Master System; 1 = Mega Drive
    n1  overscan;   //0 = 224 lines; 1 = 240 lines
    n1  verticalBlankInterruptEnable;
    n1  displayEnable;

    //$07  background color
    n6  backgroundColor;

    //$0a  horizontal interrupt counter
    n8  horizontalInterruptCounter;

    //$0b  mode register 3
    n1  externalInterruptEnable;

    //$0c  mode register 4
    n2  displayWidth;
    n2  interlaceMode;
    n1  shadowHighlightEnable;
    n1  externalColorEnable;
    n1  horizontalSync;
    n1  verticalSync;

    //$0f  data port auto-increment value
    n8  dataIncrement;
  } io;

  struct Latch {
    //per-frame
    n1 field;
    n1 interlace;
    n1 overscan;
    n8 horizontalInterruptCounter;

    //per-scanline
    n2 displayWidth;
  } latch;

  struct State {
    u32* output = nullptr;
    n16 hdot;
    n16 hcounter;
    n16 vcounter;
    n1  field;
  } state;

  friend class Interface;
};

extern VDP vdp;
#endif
