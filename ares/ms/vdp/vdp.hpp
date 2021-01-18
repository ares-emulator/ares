//Texas Instruments TMS9918A (derivative)

struct VDP : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean interframeBlending;  //Game Gear
  Memory::Writable<n8> vram;  //16KB
  Memory::Writable<n8> cram;  //SG + MS = 32, GG = 64

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory cram;
    } memory;
  } debugger;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto vlines() -> u32;
  auto vblank() -> bool;

  auto power() -> void;

  //io.cpp
  auto vcounter() -> n8;
  auto hcounter() -> n8;
  auto hcounterLatch() -> void;
  auto ccounter() -> n12;
  auto data() -> n8;
  auto status() -> n8;

  auto data(n8) -> void;
  auto control(n8) -> void;
  auto registerWrite(n4 addr, n8 data) -> void;

  //background.cpp
  struct Background {
    auto run(n8 hoffset, n9 voffset) -> void;
    auto graphics1(n8 hoffset, n9 voffset) -> void;
    auto graphics2(n8 hoffset, n9 voffset) -> void;
    auto graphics3(n8 hoffset, n9 voffset, u32 vlines) -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Output {
      n4 color;
      n1 palette;
      n1 priority;
    } output;
  } background;

  //sprite.cpp
  struct Sprite {
    auto setup(n9 voffset) -> void;
    auto run(n8 hoffset, n9 voffset) -> void;
    auto graphics1(n8 hoffset, n9 voffset) -> void;
    auto graphics2(n8 hoffset, n9 voffset) -> void;
    auto graphics3(n8 hoffset, n9 voffset, u32 vlines) -> void;

    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Object {
      n8 x;
      n8 y;
      n8 pattern;
      n4 color;
    };

    struct Output {
      n4 color;
    } output;

    array<Object[8]> objects;
    u32 objectsValid;
  } sprite;

  //color.cpp
  auto colorMasterSystem(n32) -> n64;
  auto colorGameGear(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  auto palette(n5 index) -> n12;

  struct IO {
    u32 vcounter = 0;  //vertical counter
    u32 hcounter = 0;  //horizontal counter
    u32 pcounter = 0;  //horizontal counter (latched)
    u32 lcounter = 0;  //line counter
    n12 ccounter = 0;  //csync counter

    //interrupt flags
    n1  intLine;
    n1  intFrame;

    //status flags
    n1  spriteOverflow;
    n1  spriteCollision;
    n5  fifthSprite;

    //latches
    n1  controlLatch;
    n16 controlData;
    n2  code;
    n14 address;

    n8  vramLatch;

    //$00 mode control 1
    n1  externalSync;
    n1  spriteShift;
    n1  lineInterrupts;
    n1  leftClip;
    n1  horizontalScrollLock;
    n1  verticalScrollLock;

    //$01 mode control 2
    n1  spriteDouble;
    n1  spriteTile;
    n1  frameInterrupts;
    n1  displayEnable;

    //$00 + $01
    n4  mode;

    //$02 name table base address
    n4  nameTableAddress;

    //$03 color table base address
    n8  colorTableAddress;

    //$04 pattern table base address
    n3  patternTableAddress;

    //$05 sprite attribute table base address
    n7  spriteAttributeTableAddress;

    //$06 sprite pattern table base address
    n3  spritePatternTableAddress;

    //$07 backdrop color
    n4  backdropColor;

    //$08 horizontal scroll offset
    n8  hscroll;

    //$09 vertical scroll offset
    n8  vscroll;

    //$0a line counter
    n8  lineCounter;
  } io;
};

extern VDP vdp;
