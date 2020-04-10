//Texas Instruments TMS9918A (derivative)

struct VDP : Thread {
  Node::Component node;
  Node::Screen screen;
  Node::Boolean interframeBlending;  //Game Gear
  Memory::Writable<uint8> vram;  //16KB
  Memory::Writable<uint8> cram;  //SG + MS = 32, GG = 64

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory vram;
      Node::Memory cram;
    } memory;
  } debugger;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;

  auto vlines() -> uint;
  auto vblank() -> bool;

  auto power() -> void;

  //io.cpp
  auto vcounter() -> uint8;
  auto hcounter() -> uint8;
  auto data() -> uint8;
  auto status() -> uint8;

  auto data(uint8) -> void;
  auto control(uint8) -> void;
  auto registerWrite(uint4 addr, uint8 data) -> void;

  //background.cpp
  struct Background {
    auto run(uint8 hoffset, uint9 voffset) -> void;
    auto graphics1(uint8 hoffset, uint9 voffset) -> void;
    auto graphics2(uint8 hoffset, uint9 voffset) -> void;
    auto graphics3(uint8 hoffset, uint9 voffset, uint vlines) -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Output {
      uint4 color;
      uint1 palette;
      uint1 priority;
    } output;
  } background;

  //sprite.cpp
  struct Sprite {
    auto setup(uint9 voffset) -> void;
    auto run(uint8 hoffset, uint9 voffset) -> void;
    auto graphics1(uint8 hoffset, uint9 voffset) -> void;
    auto graphics2(uint8 hoffset, uint9 voffset) -> void;
    auto graphics3(uint8 hoffset, uint9 voffset, uint vlines) -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Object {
      uint8 x;
      uint8 y;
      uint8 pattern;
      uint4 color;
    };

    struct Output {
      uint4 color;
    } output;

    array<Object[8]> objects;
    uint objectsValid;
  } sprite;

  //color.cpp
  auto colorMasterSystem(uint32) -> uint64;
  auto colorGameGear(uint32) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  auto palette(uint5 index) -> uint12;

  uint32 buffer[256 * 264];

  struct IO {
    uint vcounter = 0;  //vertical counter
    uint hcounter = 0;  //horizontal counter
    uint lcounter = 0;  //line counter

    //interrupt flags
    uint1 intLine;
    uint1 intFrame;

    //status flags
    uint1 spriteOverflow;
    uint1 spriteCollision;
    uint5 fifthSprite;

    //latches
     uint1 controlLatch;
    uint16 controlData;
     uint2 code;
    uint14 address;

    uint8 vramLatch;

    //$00 mode control 1
    uint1 externalSync;
    uint1 spriteShift;
    uint1 lineInterrupts;
    uint1 leftClip;
    uint1 horizontalScrollLock;
    uint1 verticalScrollLock;

    //$01 mode control 2
    uint1 spriteDouble;
    uint1 spriteTile;
    uint1 frameInterrupts;
    uint1 displayEnable;

    //$00 + $01
    uint4 mode;

    //$02 name table base address
    uint4 nameTableAddress;

    //$03 color table base address
    uint8 colorTableAddress;

    //$04 pattern table base address
    uint3 patternTableAddress;

    //$05 sprite attribute table base address
    uint7 spriteAttributeTableAddress;

    //$06 sprite pattern table base address
    uint3 spritePatternTableAddress;

    //$07 backdrop color
    uint4 backdropColor;

    //$08 horizontal scroll offset
    uint8 hscroll;

    //$09 vertical scroll offset
    uint8 vscroll;

    //$0a line counter
    uint8 lineCounter;
  } io;
};

extern VDP vdp;
