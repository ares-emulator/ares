struct PPU : Thread, PPUcounter {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Natural vramSize;
  Node::Setting::Boolean overscanEnable;
  Node::Setting::Boolean colorEmulation;
  Node::Setting::Boolean colorBleed;
  Node::Setting::Natural screenWidth;

  struct Debugger {
    PPU& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory oam;
      Node::Debugger::Memory cgram;
    } memory;

    struct Graphics {
      Node::Debugger::Graphics tiles2bpp;
      Node::Debugger::Graphics tiles4bpp;
      Node::Debugger::Graphics tiles8bpp;
      Node::Debugger::Graphics tilesMode7;
    } graphics;
  } debugger{*this};

  auto width() const -> u32 { return screenWidth->value(); }
  auto hires() const -> bool { return io.pseudoHires || io.bgMode == 5 || io.bgMode == 6; }
  auto interlace() const -> bool { return state.interlace; }
  auto overscan() const -> bool { return state.overscan; }
  auto vdisp() const -> u32 { return state.vdisp; }

  //ppu.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;

  auto step(u32 clocks) -> void;
  auto main() -> void;
  auto map() -> void;
  auto power(bool reset) -> void;
  auto draw(u32* output) -> void;

  //io.cpp
  auto latchCounters() -> void;
  auto addressVRAM() const -> n16;
  auto readVRAM() -> n16;
  auto writeVRAM(n1 byte, n8 data) -> void;
  auto readOAM(n10 address) -> n8;
  auto writeOAM(n10 address, n8 data) -> void;
  auto readCGRAM(n1 byte, n8 address) -> n8;
  auto writeCGRAM(n8 address, n15 data) -> void;
  auto readIO(n24 address, n8 data) -> n8;
  auto writeIO(n24 address, n8 data) -> void;
  auto updateVideoMode() -> void;

  //color.cpp
  auto color(n32 color) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Source { enum : u32 { BG1, BG2, BG3, BG4, OBJ1, OBJ2, COL }; };

  n32 renderingCycle;

  struct {
    n4 version;
    n8 mdr;
  } ppu1, ppu2;

  struct VRAM {
    auto& operator[](u32 address) { return data[address & mask]; }
    n16 data[64_KiB];
    n16 mask = 0x7fff;
    n16 address;
    n8  increment;
    n2  mapping;
    n1  mode;
  } vram;

  struct State {
    n1 interlace;
    n1 overscan;
    n9 vdisp;
  } state;

  struct Latches {
    n16 vram;
    n8  oam;
    n8  cgram;
    n8  bgofsPPU1;
    n8  bgofsPPU2;
    n8  mode7;
    n1  counters;
    n1  hcounter;
    n1  vcounter;

    n10 oamAddress;
    n8  cgramAddress;
  } latch;

  struct IO {
    //$2100  INIDISP
    n4  displayBrightness;
    n1  displayDisable;

    //$2102  OAMADDL
    //$2103  OAMADDH
    n10 oamBaseAddress;
    n10 oamAddress;
    n1  oamPriority;

    //$2105  BGMODE
    n3  bgMode;
    n1  bgPriority;

    //$2121  CGADD
    n8  cgramAddress;
    n1  cgramAddressLatch;

    //$2133  SETINI
    n1  interlace;
    n1  overscan;
    n1  pseudoHires;
    n1  extbg;

    //$213c  OPHCT
    n16 hcounter;

    //$213d  OPVCT
    n16 vcounter;
  } io;

  struct Mode7 {
    //$210d  BG1HOFS
    n16 hoffset;

    //$210e  BG1VOFS
    n16 voffset;

    //$211a  M7SEL
    n1  hflip;
    n1  vflip;
    n2  repeat;

    //$211b  M7A
    n16 a;

    //$211c  M7B
    n16 b;

    //$211d  M7C
    n16 c;

    //$211e  M7D
    n16 d;

    //$211f  M7X
    n16 hcenter;

    //$2120  M7Y
    n16 vcenter;
  } mode7;

  struct Window {
    PPU& self;
    struct Layer;
    struct Color;

    //window.cpp
    auto render(Layer&, bool enable, bool output[448]) -> void;
    auto render(Color&, u32 mask, bool output[448]) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Layer {
      n1 oneInvert;
      n1 oneEnable;
      n1 twoInvert;
      n1 twoEnable;
      n2 mask;
      n1 aboveEnable;
      n1 belowEnable;

      //serialization.cpp
      auto serialize(serializer&) -> void;
    };

    struct Color {
      n1 oneInvert;
      n1 oneEnable;
      n1 twoInvert;
      n1 twoEnable;
      n2 mask;
      n2 aboveMask;
      n2 belowMask;

      //serialization.cpp
      auto serialize(serializer&) -> void;
    };

    struct IO {
      //$2126  WH0
      n8 oneLeft;

      //$2127  WH1
      n8 oneRight;

      //$2128  WH2
      n8 twoLeft;

      //$2129  WH3
      n8 twoRight;
    } io;
  } window{*this};

  struct Mosaic {
    PPU& self;

    //mosaic.cpp
    auto enable() const -> bool;
    auto voffset() const -> u32;
    auto scanline() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n5 size;
    n5 vcounter;
  } mosaic{*this};

  struct Background {
    struct ID { enum : u32 { BG1, BG2, BG3, BG4 }; };
    struct Mode { enum : u32 { BPP2, BPP4, BPP8, Mode7, Inactive }; };

    PPU& self;
    const u32 id;
    Background(PPU& self, u32 id) : self(self), id(id) {}

    //background.cpp
    auto render() -> void;
    auto getTile(u32 hoffset, u32 voffset) -> n16;
    auto power() -> void;

    //mode7.cpp
    auto renderMode7() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n2  screenSize;
      n16 screenAddress;

      n16 tiledataAddress;
      n1  tileSize;

      n16 hoffset;
      n16 voffset;

      n1  aboveEnable;
      n1  belowEnable;
      n1  mosaicEnable;

      n8  mode;
      n8  priority[2];
    } io;

    PPU::Window::Layer window;
  };
  Background bg1{*this, Background::ID::BG1};
  Background bg2{*this, Background::ID::BG2};
  Background bg3{*this, Background::ID::BG3};
  Background bg4{*this, Background::ID::BG4};

  struct OAM {
    //oam.cpp
    auto read(n10 address) -> n8;
    auto write(n10 address, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Object {
      //oam.cpp
      auto width() const -> u32;
      auto height() const -> u32;

      n9 x;
      n8 y;
      n8 character;
      n1 nameselect;
      n1 vflip;
      n1 hflip;
      n2 priority;
      n3 palette;
      n1 size;
    } objects[128];
  };

  struct Object {
    PPU& self;

    //object.cpp
    auto addressReset() -> void;
    auto setFirstSprite() -> void;
    auto render() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    OAM oam;

    struct IO {
      n1  interlace;

      n16 tiledataAddress;
      n2  nameselect;
      n3  baseSize;
      n7  firstSprite;

      n1  aboveEnable;
      n1  belowEnable;

      n1  rangeOver;
      n1  timeOver;

      n8  priority[4];
    } io;

    PPU::Window::Layer window;

  //unserialized:
    struct Item {
      n1 valid;
      n7 index;
      n8 width;
      n8 height;
      n8 y;
    } items[32];

    struct Tile {
      n1  valid;
      n9  x;
      n2  priority;
      n8  palette;
      n1  hflip;
      n32 data;
    } tiles[34];
  } obj{*this};

  struct DAC {
    PPU& self;
    struct Pixel;

    //dac.cpp
    auto prepare() -> void;
    auto render() -> void;
    auto pixel(n9 x, Pixel above, Pixel below) const -> n15;
    auto blend(n15 x, n15 y, bool halve) const -> n15;
    auto plotAbove(n9 x, n8 source, n8 priority, n15 color) -> void;
    auto plotBelow(n9 x, n8 source, n8 priority, n15 color) -> void;
    auto directColor(n8 palette, n3 paletteGroup) const -> n15;
    auto fixedColor() const -> n15;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n15 cgram[256];

    struct IO {
      n1 directColor;
      n1 blendMode;
      n1 colorEnable[7];
      n1 colorHalve;
      n1 colorMode;
      n5 colorRed;
      n5 colorGreen;
      n5 colorBlue;
    } io;

    PPU::Window::Color window;

  //unserialized:
    struct Pixel {
      n8  source;
      n8  priority;
      n15 color;
    } above[448], below[448];

    bool windowAbove[448];
    bool windowBelow[448];
  } dac{*this};
};

extern PPU ppu;
