struct PPU : Thread, PPUcounter {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean overscanEnable;
  Node::Setting::Boolean colorEmulation;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

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
  } debugger;

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
  auto normalize() -> void;
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

private:
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

  #include "window.hpp"
  #include "mosaic.hpp"
  #include "background.hpp"
  #include "oam.hpp"
  #include "object.hpp"
  #include "dac.hpp"

  Window window;
  Mosaic mosaic;
  Background bg1{Background::ID::BG1};
  Background bg2{Background::ID::BG2};
  Background bg3{Background::ID::BG3};
  Background bg4{Background::ID::BG4};
  Object obj;
  DAC dac;

  n1  width256;
  n1  width512;
  n16 widths[240];

  friend class PPU::Window;
  friend class PPU::Mosaic;
  friend class PPU::Background;
  friend class PPU::Object;
  friend class PPU::DAC;
  friend class SuperFamicomInterface;
};

extern PPU ppu;
