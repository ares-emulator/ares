struct PPU : Thread, IO {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean colorEmulation;
  Node::Setting::Boolean interframeBlending;
  Node::Setting::String rotation;
  Memory::Writable<n8 > vram;  //96KB
  Memory::Writable<n16> pram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory pram;
    } memory;
  } debugger;

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto blank() -> bool;

  auto step(u32 clocks) -> void;
  auto main() -> void;

  auto frame() -> void;
  auto refresh() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(n32 address) -> n8;
  auto writeIO(n32 address, n8 byte) -> void;

  //memory.cpp
  auto readVRAM(u32 mode, n32 address) -> n32;
  auto writeVRAM(u32 mode, n32 address, n32 word) -> void;

  auto readPRAM(u32 mode, n32 address) -> n32;
  auto writePRAM(u32 mode, n32 address, n32 word) -> void;

  auto readOAM(u32 mode, n32 address) -> n32;
  auto writeOAM(u32 mode, n32 address, n32 word) -> void;

  auto readObjectVRAM(u32 address) const -> n8;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  //note: I/O register order is {BG0-BG3, OBJ, SFX}
  //however; layer ordering is {OBJ, BG0-BG3, SFX}
  enum : u32 { OBJ = 0, BG0 = 1, BG1 = 2, BG2 = 3, BG3 = 4, SFX = 5 };
  enum : u32 { IN0 = 0, IN1 = 1, IN2 = 2, OUT = 3 };

  struct IO {
    n1  gameBoyColorMode;
    n1  forceBlank;
    n1  greenSwap;

    n1  vblank;
    n1  hblank;
    n1  vcoincidence;
    n1  irqvblank;
    n1  irqhblank;
    n1  irqvcoincidence;
    n8  vcompare;

    n16 vcounter;
  } io;

  struct Pixel {
    n1  enable;
    n2  priority;
    n15 color;

    //OBJ only
    n1  translucent;
    n1  mosaic;
    n1  window;  //IN2
  };

  struct Background {
    //background.cpp
    auto scanline(u32 y) -> void;
    auto run(u32 x, u32 y) -> void;
    auto linear(u32 x, u32 y) -> void;
    auto affine(u32 x, u32 y) -> void;
    auto bitmap(u32 x, u32 y) -> void;
    auto power(u32 id) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    u32 id;  //BG0, BG1, BG2, BG3

    struct IO {
      static n3 mode;
      static n1 frame;
      static n5 mosaicWidth;
      static n5 mosaicHeight;

      n1 enable;

      n2 priority;
      n2 characterBase;
      n2 unused;
      n1 mosaic;
      n1 colorMode;
      n5 screenBase;
      n1 affineWrap;  //BG2, BG3 only
      n2 screenSize;

      n9 hoffset;
      n9 voffset;

      //BG2, BG3 only
      i16 pa;
      i16 pb;
      i16 pc;
      i16 pd;
      i28 x;
      i28 y;

      //internal
      i28 lx;
      i28 ly;
    } io;

    struct Latch {
      n10 character;
      n1 hflip;
      n1 vflip;
      n4 palette;
    } latch;

    Pixel output;
    Pixel mosaic;
    u32 mosaicOffset;

    u32 hmosaic;
    u32 vmosaic;

    i28 fx;
    i28 fy;
  } bg0, bg1, bg2, bg3;

  struct Objects {
    //object.cpp
    auto scanline(u32 y) -> void;
    auto run(u32 x, u32 y) -> void;
    auto power() -> void;

    //object.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1 enable;

      n1 hblank;   //1 = allow access to OAM during Hblank
      n1 mapping;  //0 = two-dimensional, 1 = one-dimensional
      n5 mosaicWidth;
      n5 mosaicHeight;
    } io;

    Pixel buffer[240];
    Pixel output;
    Pixel mosaic;
    u32 mosaicOffset;
  } objects;

  struct Window {
    //window.cpp
    auto run(u32 x, u32 y) -> void;
    auto power(u32 id) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    u32 id;  //IN0, IN1, IN2, OUT

    struct IO {
      n1 enable;
      n1 active[6];

      //IN0, IN1 only
      n8 x1;
      n8 x2;
      n8 y1;
      n8 y2;
    } io;

    n1 output;  //IN0, IN1, IN2 only
  } window0, window1, window2, window3;

  struct DAC {
    //dac.cpp
    auto run(u32 x, u32 y) -> n15;
    auto blend(n15 above, u32 eva, n15 below, u32 evb) -> n15;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n2 blendMode;
      n1 blendAbove[6];
      n1 blendBelow[6];

      n5 blendEVA;
      n5 blendEVB;
      n5 blendEVY;
    } io;
  } dac;

  struct Object {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8  y;
    n1  affine;
    n1  affineSize;
    n2  mode;
    n1  mosaic;
    n1  colors;  //0 = 16, 1 = 256
    n2  shape;   //0 = square, 1 = horizontal, 2 = vertical

    n9  x;
    n5  affineParam;
    n1  hflip;
    n1  vflip;
    n2  size;

    n10 character;
    n2  priority;
    n4  palette;

    //ancillary data
    n32 width;
    n32 height;
  } object[128];

  struct ObjectParam {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    i16 pa;
    i16 pb;
    i16 pc;
    i16 pd;
  } objectParam[32];
};

extern PPU ppu;
