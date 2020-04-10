struct PPU : Thread, PPUcounter {
  Node::Component node;
  Node::Screen screen;
  Node::Boolean overscanEnable;
  Node::Boolean colorEmulation;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory vram;
      Node::Memory oam;
      Node::Memory cgram;
    } memory;
  } debugger;

  auto hires() const -> bool { return io.pseudoHires || io.bgMode == 5 || io.bgMode == 6; }
  auto interlace() const -> bool { return state.interlace; }
  auto overscan() const -> bool { return state.overscan; }
  auto vdisp() const -> uint { return state.vdisp; }

  //ppu.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;

  auto step(uint clocks) -> void;
  auto main() -> void;
  auto map() -> void;
  auto power(bool reset) -> void;
  auto refresh() -> void;

  //io.cpp
  auto latchCounters() -> void;
  auto addressVRAM() const -> uint16;
  auto readVRAM() -> uint16;
  auto writeVRAM(uint1 byte, uint8 data) -> void;
  auto readOAM(uint10 address) -> uint8;
  auto writeOAM(uint10 address, uint8 data) -> void;
  auto readCGRAM(uint1 byte, uint8 address) -> uint8;
  auto writeCGRAM(uint8 address, uint15 data) -> void;
  auto readIO(uint24 address, uint8 data) -> uint8;
  auto writeIO(uint24 address, uint8 data) -> void;
  auto updateVideoMode() -> void;

  //color.cpp
  auto color(uint32 color) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Source { enum : uint { BG1, BG2, BG3, BG4, OBJ1, OBJ2, COL }; };

  uint32* output = nullptr;
  uint32 renderingCycle;

  struct {
    uint4 version;
    uint8 mdr;
  } ppu1, ppu2;

  struct VRAM {
    auto& operator[](uint address) { return data[address & mask]; }
    uint16 data[64_KiB];
    uint16 mask = 0x7fff;
    uint16 address;
     uint8 increment;
     uint2 mapping;
     uint1 mode;
  } vram;

  struct State {
    uint1 interlace;
    uint1 overscan;
    uint9 vdisp;
  } state;

  struct Latches {
    uint16 vram;
     uint8 oam;
     uint8 cgram;
     uint8 bgofsPPU1;
     uint8 bgofsPPU2;
     uint8 mode7;
     uint1 counters;
     uint1 hcounter;
     uint1 vcounter;

    uint10 oamAddress;
     uint8 cgramAddress;
  } latch;

  struct IO {
    //$2100  INIDISP
    uint4 displayBrightness;
    uint1 displayDisable;

    //$2102  OAMADDL
    //$2103  OAMADDH
    uint10 oamBaseAddress;
    uint10 oamAddress;
     uint1 oamPriority;

    //$2105  BGMODE
     uint3 bgMode;
     uint1 bgPriority;

    //$2121  CGADD
     uint8 cgramAddress;
     uint1 cgramAddressLatch;

    //$2133  SETINI
     uint1 interlace;
     uint1 overscan;
     uint1 pseudoHires;
     uint1 extbg;

    //$213c  OPHCT
    uint16 hcounter;

    //$213d  OPVCT
    uint16 vcounter;
  } io;

  struct Mode7 {
    //$210d  BG1HOFS
    uint16 hoffset;

    //$210e  BG1VOFS
    uint16 voffset;

    //$211a  M7SEL
     uint1 hflip;
     uint1 vflip;
     uint2 repeat;

    //$211b  M7A
    uint16 a;

    //$211c  M7B
    uint16 b;

    //$211d  M7C
    uint16 c;

    //$211e  M7D
    uint16 d;

    //$211f  M7X
    uint16 hcenter;

    //$2120  M7Y
    uint16 vcenter;
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

   uint1 width256;
   uint1 width512;
  uint16 widths[240];

  friend class PPU::Window;
  friend class PPU::Mosaic;
  friend class PPU::Background;
  friend class PPU::Object;
  friend class PPU::DAC;
  friend class SuperFamicomInterface;
};

extern PPU ppu;
