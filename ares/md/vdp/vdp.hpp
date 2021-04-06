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
    auto dma(string_view) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory vsram;
      Node::Debugger::Memory cram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification dma;
    } tracer;
  } debugger{*this};

  inline auto hdot() const -> u32 { return state.hdot; }
  inline auto hcounter() const -> u32 { return state.hcounter; }
  inline auto vcounter() const -> u32 { return state.vcounter; }
  inline auto field() const -> bool { return state.field; }
  inline auto hsync() const -> bool { return state.hsync; }
  inline auto vsync() const -> bool { return state.vsync; }

  inline auto h32() const -> bool { return io.displayWidth == 0; }  //256-width
  inline auto h40() const -> bool { return io.displayWidth == 1; }  //320-width

  inline auto dclk()  const -> bool { return io.clockSelect == 0; }  //internal clock
  inline auto edclk() const -> bool { return io.clockSelect == 1; }  //external clock

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto power(bool reset) -> void;

  //main.cpp
  auto step(u32 clocks) -> void;
  auto tick() -> void;
  auto main() -> void;
  auto hsync(bool) -> void;
  auto vsync(bool) -> void;
  auto mainH32() -> void;
  auto mainH40() -> void;
  auto mainBlankH32() -> void;
  auto mainBlankH40() -> void;

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
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1  active;
    n2  mode;
    n22 source;
    n16 length;
    n8  filldata;
    n1  enable;
    n1  wait;
  } dma;

  //render.cpp
  auto pixels() -> u32*;
  auto scanline() -> void;
  auto run() -> void;
  auto outputPixel(n32 color) -> void;

  struct Pixel {
    auto above() const -> bool { return priority == 1 && color; }
    auto below() const -> bool { return priority == 0 && color; }

    n6 color;
    n1 priority;
    n1 backdrop;
  };

  struct Layers {
    //layers.cpp
    auto hscrollFetch() -> void;
    auto vscrollFetch(u32 x) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n2  hscrollMode;
    n15 hscrollAddress;
    n1  vscrollMode;
    n2  nametableWidth;
    n2  nametableHeight;
  } layers;

  struct Attributes {
    n15 address;
    n16 width;
    n16 height;
    n10 hscroll;
    n10 vscroll;
  };

  struct Window {
    //window.cpp
    auto test(u32 x, u32 y) const -> bool;
    auto attributes() const -> Attributes;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n10 hoffset;
    n1  hdirection;
    n10 voffset;
    n1  vdirection;
    n16 nametableAddress;
  } window;

  struct Layer {
    //layer.cpp
    auto mappingFetch() -> void;
    auto patternFetch() -> void;

    auto attributes() const -> Attributes;
    auto run(u32 x, u32 y, const Attributes&) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n10 hscroll;
    n10 vscroll;
    n16 generatorAddress;
    n16 nametableAddress;
    Pixel output;
  } layerA, layerB;

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
    n1  hflip;
    n1  vflip;
    n2  palette;
    n1  priority;
    n11 address;
    n7  link;
  };

  struct Sprite {
    VDP& vdp;

    //the per-scanline sprite limits are different between H40 and H32 modes
    auto objectLimit() const -> u32 { return vdp.latch.displayWidth ? 20 : 16; }
    auto tileLimit()   const -> u32 { return vdp.latch.displayWidth ? 40 : 32; }
    auto linkLimit()   const -> u32 { return vdp.latch.displayWidth ? 80 : 64; }

    //sprite.cpp
    auto write(n9 address, n16 data) -> void;
    auto mappingFetch() -> void;
    auto patternFetch() -> void;
    auto scanline(u32 y) -> void;
    auto run(u32 x, u32 y) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n16 generatorAddress;
    n16 nametableAddress;
    n1  collision;
    n1  overflow;
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

  struct FIFO {
    //fifo.cpp
    auto slot() -> void;
    auto read(n4 target, n17 address) -> void;
    auto write(n4 target, n17 address, n8 data) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Slot {
      //serialization.cpp
      auto serialize(serializer&) -> void;

      n4  target;
      n17 address;
      n8  data;
    };

    queue<Slot[8]> slots;
    queue<Slot[2]> requests;
    n16 response;
  } fifo;

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
    n1  vblankInterruptTriggered;  //true after VIRQ triggers; cleared at start of next frame

    //command
    n6  command;
    n17 address;
    n1  commandPending;

    //$00  mode register 1
    n1  displayOverlayEnable;
    n1  counterLatch;
    n1  hblankInterruptEnable;
    n1  leftColumnBlank;

    //$01  mode register 2
    n1  videoMode;  //0 = Master System; 1 = Mega Drive
    n1  overscan;   //0 = 224 lines; 1 = 240 lines
    n1  vblankInterruptEnable;
    n1  displayEnable;

    //$07  background color
    n6  backgroundColor;

    //$0a  horizontal interrupt counter
    n8  hblankInterruptCounter;

    //$0b  mode register 3
    n1  externalInterruptEnable;

    //$0c  mode register 4
    n1  displayWidth;  //0 = H32; 1 = H40
    n2  interlaceMode;
    n1  shadowHighlightEnable;
    n1  externalColorEnable;
    n1  hsync;
    n1  vsync;
    n1  clockSelect;  //0 = DCLK; 1 = EDCLK

    //$0f  data port auto-increment value
    n8  dataIncrement;
  } io;

  struct Latch {
    //per-frame
    n1 interlace;
    n1 overscan;

    //per-scanline
    n8 hblankInterruptCounter;
    n1 displayWidth;
    n1 clockSelect;
  } latch;

  struct State {
    u32* output = nullptr;
    n16  hdot;
    n16  hcounter;
    n16  vcounter;
    n16  ecounter;
    n1   field;
    n1   hsync;
    n1   vsync;
  } state;
};

extern VDP vdp;
#endif
