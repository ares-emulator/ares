#if defined(PROFILE_PERFORMANCE)
#include "../ppu-performance/ppu.hpp"
#else
struct PPU : Thread, PPUcounter {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Natural versionPPU1;
  Node::Setting::Natural versionPPU2;
  Node::Setting::Natural vramSize;
  Node::Setting::Boolean overscanEnable;
  Node::Setting::Boolean colorEmulation;
  Node::Setting::Boolean colorBleed;

  struct Debugger {
    PPU& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto registers() -> string;

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

    struct Properties {
      Node::Debugger::Properties registers;
    } properties;
  } debugger{*this};

  auto interlace() const -> bool { return self.interlace; }
  auto overscan() const -> bool { return self.overscan; }
  auto vdisp() const -> u32 { return self.vdisp; }

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto map() -> void;
  auto power(bool reset) -> void;

  //main.cpp
  auto main() -> void;
  noinline auto cycleObjectEvaluate() -> void;
  template<u32 Cycle> noinline auto cycleBackgroundFetch() -> void;
  noinline auto cycleBackgroundBegin() -> void;
  noinline auto cycleBackgroundBelow() -> void;
  noinline auto cycleBackgroundAbove() -> void;
  noinline auto cycleRenderPixel() -> void;
  template<u32> auto cycle() -> void;

  //io.cpp
  auto latchCounters() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  //ppu.cpp
  auto step() -> void;
  auto step(u32 clocks) -> void;

  //io.cpp
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

  struct VRAM {
    auto& operator[](u32 address) { return data[address & mask]; }
    n16 data[64_KiB];
    n16 mask = 0x7fff;
  } vram;

  struct OAM {
    //oam.cpp
    auto read(n10 address) -> n8;
    auto write(n10 address, n8 data) -> void;

    struct Object {
      //oam.cpp
      auto width() const -> uint;
      auto height() const -> uint;

      //serialization.cpp
      auto serialize(serializer&) -> void;

      n9 x;
      n8 y;
      n8 character;
      n1 nameselect;
      n1 vflip;
      n1 hflip;
      n2 priority;
      n3 palette;
      n1 size;
    } object[128];
  } oam;

  n15 cgram[256];

  struct {
    n1 interlace;
    n1 overscan;
    n9 vdisp;
  } self;

  struct {
    n4 version;
    n8 mdr;
  } ppu1, ppu2;

  struct Latch {
    n16 vram;
    n8  oam;
    n8  cgram;
    n8  bgofsPPU1;
    n3  bgofsPPU2;
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

    //$210d  BG1HOFS
    n16 hoffsetMode7;

    //$210e  BG1VOFS
    n16 voffsetMode7;

    //$2115  VMAIN
    n8  vramIncrementSize;
    n2  vramMapping;
    n1  vramIncrementMode;

    //$2116  VMADDL
    //$2117  VMADDH
    n16 vramAddress;

    //$211a  M7SEL
    n1  hflipMode7;
    n1  vflipMode7;
    n2  repeatMode7;

    //$211b  M7A
    n16 m7a;

    //$211c  M7B
    n16 m7b;

    //$211d  M7C
    n16 m7c;

    //$211e  M7D
    n16 m7d;

    //$211f  M7X
    n16 m7x;

    //$2120  M7Y
    n16 m7y;

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

    PPU& self;
    VRAM& vram;
    const u32 id;

    //background.cpp
    auto hires() const -> bool;
    auto frame() -> void;
    auto scanline() -> void;
    auto begin() -> void;
    auto fetchNameTable() -> void;
    auto fetchOffset(u32 y) -> void;
    auto fetchCharacter(u32 index, bool half = 0) -> void;
    auto run(bool screen) -> void;
    auto power() -> void;

    //mode7.cpp
    auto runMode7() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Mode { enum : u32 { BPP2, BPP4, BPP8, Mode7, Inactive }; };
    struct ScreenSize { enum : u32 { Size32x32, Size32x64, Size64x32, Size64x64 }; };
    struct TileSize { enum : u32 { Size8x8, Size16x16 }; };
    struct Screen { enum : u32 { Above, Below }; };

    struct IO {
      n2  screenSize;
      n16 screenAddress;

      n16 tiledataAddress;
      n1  tileSize;

      n8  mode;
      n8  priority[2];

      n1  aboveEnable;
      n1  belowEnable;

      n16 hoffset;
      n16 voffset;
    } io;

    struct Pixel {
      n8 priority;  //0 = none (transparent)
      n8 palette;
      n3 paletteGroup;
    } above, below;

    struct Output {
      Pixel above;
      Pixel below;
    } output;

    struct Mosaic {
      n1  enable;
      n16 hcounter;
      n16 hoffset;
      Pixel pixel;
    } mosaic;

    struct OffsetPerTile {
      //set in BG3 only; used by BG1 and BG2
      n16 hoffset;
      n16 voffset;
    } opt;

    struct Tile {
      n16 address;
      n10 character;
      n8  palette;
      n3  paletteGroup;
      n8  priority;
      n1  hmirror;
      n1  vmirror;
      n16 data[4];
    } tiles[66];

    n7 renderingIndex;
    n3 pixelCounter;
  };
  Background bg1{*this, vram, Background::ID::BG1};
  Background bg2{*this, vram, Background::ID::BG2};
  Background bg3{*this, vram, Background::ID::BG3};
  Background bg4{*this, vram, Background::ID::BG4};

  struct Object {
    PPU& self;
    VRAM& vram;
    OAM& oam;

    //object.cpp
    auto addressReset() -> void;
    auto setFirstSprite() -> void;
    auto frame() -> void;
    auto scanline() -> void;
    auto evaluate(n7 index) -> void;
    auto run() -> void;
    auto fetch() -> void;
    auto power() -> void;

    auto onScanline(PPU::OAM::Object&) -> bool;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1  aboveEnable;
      n1  belowEnable;
      n1  interlace;

      n16 tiledataAddress;
      n2  nameselect;
      n3  baseSize;
      n7  firstSprite;

      n8  priority[4];

      n1  rangeOver;
      n1  timeOver;
    } io;

    struct Latch {
      n7 firstSprite;
    } latch;

    struct Item {
      n1 valid;
      n7 index;
    };

    struct Tile {
      n1  valid;
      n9  x;
      n2  priority;
      n8  palette;
      n1  hflip;
      n32 data;
    };

    struct State {
      u32  x;
      u32  y;

      u32  itemCount;
      u32  tileCount;

      bool active;
      Item item[2][32];
      Tile tile[2][34];
    } t;

    struct Output {
      struct Pixel {
        n8 priority;  //0 = none (transparent)
        n8 palette;
      } above, below;
    } output;
  } obj{*this, vram, oam};

  struct Window {
    PPU& self;
    Background& bg1;
    Background& bg2;
    Background& bg3;
    Background& bg4;
    Object& obj;

    //window.cpp
    auto scanline() -> void;
    auto run() -> void;
    auto test(bool oneEnable, bool one, bool twoEnable, bool two, u32 mask) -> bool;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      struct Layer {
        n1 oneInvert;
        n1 oneEnable;
        n1 twoInvert;
        n1 twoEnable;
        n2 mask;
        n1 aboveEnable;
        n1 belowEnable;
      } bg1, bg2, bg3, bg4, obj;

      struct Color {
        n1 oneEnable;
        n1 oneInvert;
        n1 twoEnable;
        n1 twoInvert;
        n2 mask;
        n2 aboveMask;
        n2 belowMask;
      } col;

      n8 oneLeft;
      n8 oneRight;
      n8 twoLeft;
      n8 twoRight;
    } io;

    struct Output {
      struct Pixel {
        n1 colorEnable;
      } above, below;
    } output;

    struct {
      u32 x;
    };
  } window{*this, bg1, bg2, bg3, bg4, obj};

  struct DAC {
    PPU& self;
    Background& bg1;
    Background& bg2;
    Background& bg3;
    Background& bg4;
    Object& obj;
    Window& window;
    n15* cgram;

    //dac.cpp
    auto scanline() -> void;
    auto run() -> void;
    auto power() -> void;

    auto below(bool hires) -> n16;
    auto above() -> n16;

    auto blend(u32 x, u32 y) const -> n15;
    auto paletteColor(n8 palette) const -> n15;
    auto directColor(n8 palette, n3 paletteGroup) const -> n15;
    auto fixedColor() const -> n15;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1 directColor;
      n1 blendMode;

      struct Layer {
        n1 colorEnable;
      } bg1, bg2, bg3, bg4, obj, back;
      n1 colorHalve;
      n1 colorMode;

      n5 colorRed;
      n5 colorGreen;
      n5 colorBlue;
    } io;

    struct Math {
      struct Screen {
        n15 color;
        n1  colorEnable;
      } above, below;
      n1 transparent;
      n1 blendMode;
      n1 colorHalve;
    } math;

  //unserialized:
    u32* line = nullptr;
  } dac{*this, bg1, bg2, bg3, bg3, obj, window, cgram};
};

extern PPU ppu;
#endif
