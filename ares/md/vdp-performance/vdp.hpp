//Yamaha YM7101
struct VDP : Thread {
  Node::Component node;
  Node::Screen screen;
  Node::Boolean overscan;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory vram;
      Node::Memory vsram;
      Node::Memory cram;
    } memory;
  } debugger;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;
  auto render() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto read(uint24 address, uint16 data) -> uint16;
  auto write(uint24 address, uint16 data) -> void;

  auto readDataPort() -> uint16;
  auto writeDataPort(uint16 data) -> void;

  auto readControlPort() -> uint16;
  auto writeControlPort(uint16 data) -> void;

  //color.cpp
  auto color(uint32) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  auto pixelWidth() const -> uint { return latch.displayWidth ? 4 : 5; }
  auto screenWidth() const -> uint { return latch.displayWidth ? 320 : 256; }
  auto screenHeight() const -> uint { return latch.overscan ? 240 : 224; }
  auto frameHeight() const -> uint { return Region::PAL() ? 312 : 262; }

  uint32 buffer[320 * 512];
  uint32* output = nullptr;

  struct VRAM {
    //memory.cpp
    auto read(uint16 address) const -> uint16;
    auto write(uint16 address, uint16 data) -> void;

    auto readByte(uint17 address) const -> uint8;
    auto writeByte(uint17 address, uint8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

     uint8 pixels[131072];
    uint16 memory[65536];
    uint32 size = 32768;
     uint1 mode;  //0 = 64KB, 1 = 128KB
  } vram;

  struct VSRAM {
    //memory.cpp
    auto read(uint6 address) const -> uint10;
    auto write(uint6 address, uint10 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint10 memory[40];
  } vsram;

  struct CRAM {
    //memory.cpp
    auto read(uint6 address) const -> uint9;
    auto write(uint6 address, uint9 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

     uint9 memory[64];
    uint32 palette[3 * 128];
  } cram;

  struct DMA {
    //dma.cpp
    auto run() -> void;
    auto load() -> void;
    auto fill() -> void;
    auto copy() -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint1 active;

    struct IO {
       uint2 mode;
      uint22 source;
      uint16 length;
       uint8 fill;
       uint1 enable;
       uint1 wait;
    } io;
  } dma;

  struct Background {
    enum class ID : uint { PlaneA, Window, PlaneB } id;

    //background.cpp
    auto renderScreen(uint from, uint to) -> void;
    auto renderWindow(uint from, uint to) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      uint16 generatorAddress;
      uint16 nametableAddress;

      //PlaneA, PlaneB
       uint2 nametableWidth;
       uint2 nametableHeight;
      uint15 horizontalScrollAddress;
       uint2 horizontalScrollMode;
       uint1 verticalScrollMode;

      //Window
      uint10 horizontalOffset;
       uint1 horizontalDirection;
      uint10 verticalOffset;
       uint1 verticalDirection;
    } io;

  //unserialized:
    uint7 pixels[320];
  };
  Background planeA{Background::ID::PlaneA};
  Background window{Background::ID::Window};
  Background planeB{Background::ID::PlaneB};

  struct Object {
    //object.cpp
    auto width() const -> uint;
    auto height() const -> uint;

    //serialization.cpp
    auto serialize(serializer&) -> void;

     uint9 x;
    uint10 y;
     uint2 tileWidth;
     uint2 tileHeight;
     uint1 horizontalFlip;
     uint1 verticalFlip;
     uint2 palette;
     uint1 priority;
    uint11 address;
     uint7 link;
  };

  struct Sprite {
    //sprite.cpp
    auto render() -> void;
    auto write(uint9 address, uint16 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      uint16 generatorAddress;
      uint16 nametableAddress;
    } io;

    Object oam[80];
    Object objects[20];

  //unserialized:
    uint7 pixels[512];
  } sprite;

  struct State {
    uint16 hdot;
    uint16 hcounter;
    uint16 vcounter;
     uint1 field;
  } state;

  struct IO {
    //status
     uint1 vblankIRQ;  //true after VIRQ triggers; cleared at start of next frame

    //command
     uint6 command;
    uint17 address;
     uint1 commandPending;

    //$00  mode register 1
     uint1 displayOverlayEnable;
     uint1 counterLatch;
     uint1 horizontalBlankInterruptEnable;
     uint1 leftColumnBlank;

    //$01  mode register 2
     uint1 videoMode;  //0 = Master System; 1 = Mega Drive
     uint1 overscan;   //0 = 224 lines; 1 = 240 lines
     uint1 verticalBlankInterruptEnable;
     uint1 displayEnable;

    //$07  background color
     uint7 backgroundColor;

    //$0a  horizontal interrupt counter
     uint8 horizontalInterruptCounter;

    //$0b  mode register 3
     uint1 externalInterruptEnable;

    //$0c  mode register 4
     uint2 displayWidth;
     uint2 interlaceMode;
     uint1 shadowHighlightEnable;
     uint1 externalColorEnable;
     uint1 horizontalSync;
     uint1 verticalSync;

    //$0f  data port auto-increment value
     uint8 dataIncrement;
  } io;

  struct Latch {
    //per-frame
     uint1 interlace;
     uint1 overscan;
     uint8 horizontalInterruptCounter;

    //per-scanline
     uint2 displayWidth;
  } latch;

//unserialized:
  uint8 lookupBG[1 << 10];
  uint8 lookupFG[1 << 15];

  friend class CPU;
  friend class APU;
};

extern VDP vdp;
