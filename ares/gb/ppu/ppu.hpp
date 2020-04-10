struct PPU : Thread {
  Node::Component node;
  Node::Screen screen;
  Node::String colorEmulationDMG;
  Node::Boolean colorEmulationCGB;
  Node::Boolean interframeBlending;
  Memory::Writable<uint8> vram;  //GB = 8KB, GBC = 16KB
  Memory::Writable<uint8> oam;
  uint8 bgp[4];
  uint8 obp[2][4];
  uint8 bgpd[64];
  uint8 obpd[64];

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory vram;
      Node::Memory oam;
    } memory;
  } debugger;

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto mode(uint2 mode) -> void;
  auto stat() -> void;
  auto coincidence() -> bool;
  auto refresh() -> void;
  auto step(uint clocks) -> void;

  auto hflip(uint data) const -> uint;

  auto power() -> void;

  //timing.cpp
  auto canAccessVRAM() const -> bool;
  auto canAccessOAM() const -> bool;
  auto compareLYC() const -> bool;

  //io.cpp
  auto vramAddress(uint13 address) const -> uint16;
  auto readIO(uint cycle, uint16 address, uint8 data) -> uint8;
  auto writeIO(uint cycle, uint16 address, uint8 data) -> void;

  //dmg.cpp
  auto readTileDMG(bool select, uint x, uint y, uint& data) -> void;
  auto scanlineDMG() -> void;
  auto runDMG() -> void;
  auto runBackgroundDMG() -> void;
  auto runWindowDMG() -> void;
  auto runObjectsDMG() -> void;

  //cgb.cpp
  auto readTileCGB(bool select, uint x, uint y, uint& attr, uint& data) -> void;
  auto scanlineCGB() -> void;
  auto runCGB() -> void;
  auto runBackgroundCGB() -> void;
  auto runWindowCGB() -> void;
  auto runObjectsCGB() -> void;

  //color.cpp
  auto colorGameBoy(uint32) -> uint64;
  auto colorGameBoyColor(uint32) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  function<void ()> scanline;
  function<void ()> run;

  uint32 output[160 * 144];

  struct Status {
    uint1 irq;  //STAT IRQ line
    uint9 lx;   //0~455

    //$ff40  LCDC
    uint1 bgEnable;
    uint1 obEnable;
    uint1 obSize;
    uint1 bgTilemapSelect;
    uint1 bgTiledataSelect;
    uint1 windowDisplayEnable;
    uint1 windowTilemapSelect;
    uint1 displayEnable;

    //$ff41  STAT
    uint2 mode;  //0 = Hblank, 1 = Vblank, 2 = OAM search, 3 = LCD transfer
    uint1 interruptHblank;
    uint1 interruptVblank;
    uint1 interruptOAM;
    uint1 interruptLYC;

    //$ff42  SCY
    uint8 scy;

    //$ff43  SCX
    uint8 scx;

    //$ff44  LY
    uint8 ly;

    //$ff45  LYC
    uint8 lyc;

    //$ff46  DMA
     uint8 dmaBank;
     uint1 dmaActive;
    uint10 dmaClock;  //0~323 (DMG), 0~645 (CGB)

    //$ff4a  WY
    uint8 wy;

    //$ff4b  WX
    uint8 wx;

    //$ff4f  VBK
    uint1 vramBank;

    //$ff68  BGPI
    uint6 bgpi;
    uint1 bgpiIncrement;

    //$ff6a  OBPI
    uint8 obpi;
    uint1 obpiIncrement;
  } status;

  struct Latch {
    uint1 displayEnable;
    uint1 windowDisplayEnable;
    uint8 wx;
    uint8 wy;
  } latch;

  struct History {
    uint10 mode;  //5 x 2-bit
  } history;

  struct Pixel {
    uint16 color;
     uint8 palette;
     uint1 priority;
  };
  Pixel bg;
  Pixel ob;

  struct Sprite {
    uint x = 0;
    uint y = 0;
    uint tile = 0;
    uint attr = 0;
    uint data = 0;
  };
  Sprite sprite[10];
  uint4 sprites;  //0~9

  uint8 px;  //0~159

  struct Background {
    uint attr = 0;
    uint data = 0;
  };
  Background background;
  Background window;
};

extern PPU ppu;
