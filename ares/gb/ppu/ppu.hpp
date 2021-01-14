struct PPU : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::String colorEmulationDMG;
  Node::Setting::Boolean colorEmulationCGB;
  Node::Setting::Boolean interframeBlending;
  Memory::Writable<uint8> vram;  //GB = 8KB, GBC = 16KB
  Memory::Writable<uint8> oam;
  Memory::Writable<uint2> bgp;
  Memory::Writable<uint2> obp;
  Memory::Writable<uint16> bgpd;
  Memory::Writable<uint16> obpd;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory oam;
      Node::Debugger::Memory bgp;
      Node::Debugger::Memory obp;
      Node::Debugger::Memory bgpd;
      Node::Debugger::Memory obpd;
    } memory;
  } debugger;

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto mode(uint2 mode) -> void;
  auto stat() -> void;
  auto coincidence() -> bool;
  auto step(uint clocks) -> void;

  auto hflip(uint16 tiledata) const -> uint16;

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
  auto readTileDMG(bool select, uint x, uint y, uint16& tiledata) -> void;
  auto scanlineDMG() -> void;
  auto runDMG() -> void;
  auto runBackgroundDMG() -> void;
  auto runWindowDMG() -> void;
  auto runObjectsDMG() -> void;

  //cgb.cpp
  auto readTileCGB(bool select, uint x, uint y, uint16& tiledata, uint8& attributes) -> void;
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

  struct Status {
    uint1 irq;  //STAT IRQ line
    uint9 lx;   //0~455

    //$ff40  LCDC
    uint1 bgEnable;  //DMG: BG enable; CGB: BG priority
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
    uint15 color;
     uint8 palette;
     uint1 priority;
  };
  Pixel bg;
  Pixel ob;

  struct Sprite {
     int16 x;
     int16 y;
     uint8 tile;
     uint8 attributes;
    uint16 tiledata;
  };
  Sprite sprite[10];
  uint4 sprites;  //0-9

  uint8 px;  //0-159

  struct Background {
     uint8 attributes;
    uint16 tiledata;
  };
  Background background;
  Background window;
};

extern PPU ppu;
