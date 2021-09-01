struct PPU : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::String colorEmulationDMG;
  Node::Setting::Boolean colorEmulationCGB;
  Node::Setting::Boolean interframeBlending;
  Memory::Writable<n8 > vram;  //GB = 8KB, GBC = 16KB
  Memory::Writable<n8 > oam;
  Memory::Writable<n2 > bgp;
  Memory::Writable<n2 > obp;
  Memory::Writable<n16> bgpd;
  Memory::Writable<n16> obpd;

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
  auto mode(n2 mode) -> void;
  auto stat() -> void;
  auto step(u32 clocks) -> void;

  auto hflip(n16 tiledata) const -> n16;

  auto power() -> void;

  //timing.cpp
  auto canAccessVRAM() const -> bool;
  auto canAccessOAM() const -> bool;
  auto compareLYC() const -> bool;
  auto getLY() const -> n8;
  auto triggerOAM() const -> bool;

  //io.cpp
  auto vramAddress(n13 address) const -> n16;
  auto readIO(u32 cycle, n16 address, n8 data) -> n8;
  auto writeIO(u32 cycle, n16 address, n8 data) -> void;

  //dmg.cpp
  auto readTileDMG(bool select, u32 x, u32 y, n16& tiledata) -> void;
  auto scanlineDMG() -> void;
  auto runDMG() -> void;
  auto runBackgroundDMG() -> void;
  auto runWindowDMG() -> void;
  auto runObjectsDMG() -> void;

  //cgb.cpp
  auto readTileCGB(bool select, u32 x, u32 y, n16& tiledata, n8& attributes) -> void;
  auto scanlineCGB() -> void;
  auto runCGB() -> void;
  auto runBackgroundCGB() -> void;
  auto runWindowCGB() -> void;
  auto runObjectsCGB() -> void;

  //color.cpp
  auto colorGameBoy(n32) -> n64;
  auto colorGameBoyColor(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  function<void ()> scanline;
  function<void ()> run;

  struct Status {
    n1 irq;  //STAT IRQ line
    n9 lx;   //0~455

    //$ff40  LCDC
    n1 bgEnable;  //DMG: BG enable; CGB: BG priority
    n1 obEnable;
    n1 obSize;
    n1 bgTilemapSelect;
    n1 bgTiledataSelect;
    n1 windowDisplayEnable;
    n1 windowTilemapSelect;
    n1 displayEnable;

    //$ff41  STAT
    n2 mode;  //0 = Hblank, 1 = Vblank, 2 = OAM search, 3 = LCD transfer
    n1 interruptHblank;
    n1 interruptVblank;
    n1 interruptOAM;
    n1 interruptLYC;

    //$ff42  SCY
    n8 scy;

    //$ff43  SCX
    n8 scx;

    //$ff44  LY
    n8 ly;

    //$ff45  LYC
    n8 lyc;

    //$ff46  DMA
    n8  dmaBank;
    n1  dmaActive;
    n10 dmaClock;  //0~323 (DMG), 0~645 (CGB)

    //$ff4a  WY
    n8 wy;

    //$ff4b  WX
    n8 wx;

    //$ff4f  VBK
    n1 vramBank;

    //$ff68  BGPI
    n6 bgpi;
    n1 bgpiIncrement;

    //$ff6a  OBPI
    n8 obpi;
    n1 obpiIncrement;
  } status;

  struct Latch {
    n1 displayEnable;
    n1 windowDisplayEnable;
    n8 wx;
    n8 wy;
  } latch;

  struct History {
    n10 mode;  //5 x 2-bit
  } history;

  struct Pixel {
    n15 color;
    n8  palette;
    n1  priority;
  };
  Pixel bg;
  Pixel ob;

  struct Sprite {
    i16 x;
    i16 y;
    n8  tile;
    n8  attributes;
    n16 tiledata;
  };
  Sprite sprite[10];
  n4 sprites;  //0-9

  n8 px;  //0-159

  struct Background {
    n8  attributes;
    n16 tiledata;
  };
  Background background;
  Background window;
};

extern PPU ppu;
