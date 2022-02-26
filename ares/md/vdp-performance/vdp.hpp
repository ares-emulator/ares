//Yamaha YM7101
struct VDP : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean overscan;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory vsram;
      Node::Debugger::Memory cram;
    } memory;
  } debugger;

  auto hcounter() const -> u32 { return state.hcounter; }
  auto vcounter() const -> u32 { return state.vcounter; }
  auto field() const -> bool { return state.field; }
  auto hblank() const -> bool { return state.hblank; }
  auto vblank() const -> bool { return state.vblank; }
  auto refreshing() const -> bool { return false; }

  auto h32() const -> bool { return latch.displayWidth == 0; }  //256-width
  auto h40() const -> bool { return latch.displayWidth == 1; }  //320-width

  auto v28() const -> bool { return io.overscan == 0; }  //224-height
  auto v30() const -> bool { return io.overscan == 1; }  //240-height

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto frame() -> void;
  auto power(bool reset) -> void;

  //main.cpp
  auto step(u32 clocks) -> void;
  auto vtick() -> void;
  auto hblank(bool line) -> void;
  auto vblank(bool line) -> void;
  auto vedge() -> void;
  auto main() -> void;

  //render.cpp
  auto pixels() -> u32*;
  auto scanline() -> void;
  auto render() -> void;
  auto outputPixel(n32 color) -> void;

  //io.cpp
  auto read(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void;

  auto readDataPort() -> n16;
  auto writeDataPort(n16 data) -> void;

  auto readControlPort() -> n16;
  auto writeControlPort(n16 data) -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct PSG : SN76489, Thread {
    Node::Object node;
    Node::Audio::Stream stream;

    //psg.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto main() -> void;
    auto step(u32 clocks) -> void;

    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  private:
    double volume[16];
  } psg;

  struct IRQ {
    //irq.cpp
    auto poll() -> void;
    auto acknowledge(u8 level) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct External {
      n1 enable;
      n1 pending;
    } external;

    struct Hblank {
      n1 enable;
      n1 pending;
      n8 counter;
      n8 frequency;
    } hblank;

    struct Vblank {
      n1 enable;
      n1 pending;
      n1 transitioned;
    } vblank;
  } irq;

  struct DMA {
    //dma.cpp
    auto poll() -> void;
    auto run() -> bool;
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

private:
  auto pixelWidth() const -> u32 { return latch.displayWidth ? 4 : 5; }
  auto screenWidth() const -> u32 { return latch.displayWidth ? 320 : 256; }
  auto screenHeight() const -> u32 { return latch.overscan ? 240 : 224; }
  auto frameHeight() const -> u32 { return Region::PAL() ? 312 : 262; }

  struct VRAM {
    //memory.cpp
    auto read(n16 address) const -> n16;
    auto write(n16 address, n16 data) -> void;

    auto readByte(n17 address) const -> n8;
    auto writeByte(n17 address, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8  pixels[131072];
    n16 memory[65536];
    n32 size = 32768;
    n1  mode;  //0 = 64KB, 1 = 128KB
  } vram;

  struct VSRAM {
    //memory.cpp
    auto read(n6 address) const -> n10;
    auto write(n6 address, n10 data) -> void;

    auto readByte(n7 address) const -> n8;
    auto writeByte(n7 address, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n10 memory[40];
  } vsram;

  struct CRAM {
    //memory.cpp
    auto read(n6 address) const -> n9;
    auto write(n6 address, n9 data) -> void;

    auto readByte(n7 address) const -> n8;
    auto writeByte(n7 address, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n9 memory[64];
  } cram;

  struct Pixel {
    auto above() const -> bool { return priority == 1 && color; }
    auto below() const -> bool { return priority == 0 && color; }

    n6 color;
    n1 priority;
    n1 backdrop;
  };

  struct Background {
    enum class ID : u32 { PlaneA, Window, PlaneB } id;

    //background.cpp
    auto renderScreen(u32 from, u32 to) -> void;
    auto renderWindow(u32 from, u32 to) -> void;

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

  //unserialized:
    Pixel pixels[320];
  };
  Background planeA{Background::ID::PlaneA};
  Background window{Background::ID::Window};
  Background planeB{Background::ID::PlaneB};

  struct Object {
    //object.cpp
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

    auto objectLimit() const -> u32 { return vdp.io.displayWidth ? 20 : 16; }
    auto tileLimit()   const -> u32 { return vdp.io.displayWidth ? 40 : 32; }
    auto linkLimit()   const -> u32 { return vdp.io.displayWidth ? 80 : 64; }

    //sprite.cpp
    auto render() -> void;
    auto write(n9 address, n16 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n16 generatorAddress;
      n16 nametableAddress;
    } io;

    Object oam[80];
    Object objects[20];

  //unserialized:
    Pixel pixels[512];
  } sprite{*this};

  struct Command {
    n1  latch;
    n4  target;
    n1  ready;
    n1  pending;
    n17 address;
    n8  increment;
  } command;

  struct IO {
    //$00  mode register 1
    n1 displayOverlayEnable;
    n1 counterLatch;
    n1 videoMode4;
    n1 leftColumnBlank;

    //$01  mode register 2
    n1 videoMode5;
    n1 overscan;
    n1 displayEnable;

    //$07  background color
    n6 backgroundColor;

    //$0c  mode register 4
    n1 displayWidth;  //0 = H32; 1 = H40
    n2 interlaceMode;
    n1 shadowHighlightEnable;
    n1 externalColorEnable;
    n1 hsync;
    n1 vsync;
    n1 clockSelect;  //0 = DCLK; 1 = EDCLK
  } io;

  struct Latch {
    //per-frame
    n1 interlace;
    n1 overscan;

    //per-scanline
    n1 displayWidth;
    n1 clockSelect;
  } latch;

  struct State {
    u32* output = nullptr;
    n8   hcounter;
    n9   vcounter;
    n1   field;
    n1   hblank;
    n1   vblank;
  } state;
};

extern VDP vdp;
