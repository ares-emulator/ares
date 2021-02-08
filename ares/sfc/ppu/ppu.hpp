#if defined(PROFILE_PERFORMANCE)
#include "../ppu-performance/ppu.hpp"
#else
struct PPU : Thread, PPUcounter {
  Node::Object node;
  Node::Setting::Natural versionPPU1;
  Node::Setting::Natural versionPPU2;
  Node::Setting::Natural vramSize;
  Node::Video::Screen screen;
  Node::Setting::Boolean overscanEnable;
  Node::Setting::Boolean colorEmulation;
  Node::Setting::Boolean colorBleed;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
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
  } debugger;

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

private:
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

  struct {
    n1 interlace;
    n1 overscan;
    n9 vdisp;
  } self;

  struct {
    n4 version;
    n8 mdr;
  } ppu1, ppu2;

  struct Latches {
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

  #include "mosaic.hpp"
  #include "background.hpp"
  #include "oam.hpp"
  #include "object.hpp"
  #include "window.hpp"
  #include "dac.hpp"

  Mosaic mosaic;
  Background bg1{Background::ID::BG1};
  Background bg2{Background::ID::BG2};
  Background bg3{Background::ID::BG3};
  Background bg4{Background::ID::BG4};
  Object obj;
  Window window;
  DAC dac;

  friend class PPU::Background;
  friend class PPU::Object;
  friend class PPU::Window;
  friend class PPU::DAC;
  friend class SuperFamicomInterface;
};

extern PPU ppu;
#endif
