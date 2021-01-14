struct PPU : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean overscan;
  Node::Setting::Boolean colorEmulation;
  Memory::Writable<n8> ciram;
  Memory::Writable<n8> cgram;
  Memory::Writable<n8> oam;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    struct Memory {
      Node::Debugger::Memory ciram;
      Node::Debugger::Memory cgram;
      Node::Debugger::Memory oam;
    } memory;
  } debugger;

  auto rate() const -> u32 { return Region::PAL() ? 5 : 4; }
  auto vlines() const -> u32 { return Region::PAL() ? 312 : 262; }

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto scanline() -> void;
  auto frame() -> void;

  auto power(bool reset) -> void;

  //memory.cpp
  auto readCIRAM(n11 address) -> n8;
  auto writeCIRAM(n11 address, n8 data) -> void;

  auto readCGRAM(n5 address) -> n8;
  auto writeCGRAM(n5 address, n8 data) -> void;

  auto readIO(n16 address) -> n8;
  auto writeIO(n16 address, n8 data) -> void;

  //render.cpp
  auto enable() const -> bool;
  auto loadCHR(n16 address) -> n8;

  auto renderPixel() -> void;
  auto renderSprite() -> void;
  auto renderScanline() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    //internal
    n08 mdr;
    n01 field;
    n16 lx;
    n16 ly;
    n08 busData;

    struct Union {
      n19 data;
      BitRange<19, 0, 4> tileX     {&data};
      BitRange<19, 5, 9> tileY     {&data};
      BitRange<19,10,11> nametable {&data};
      BitRange<19,10,10> nametableX{&data};
      BitRange<19,11,11> nametableY{&data};
      BitRange<19,12,14> fineY     {&data};
      BitRange<19, 0,14> address   {&data};
      BitRange<19, 0, 7> addressLo {&data};
      BitRange<19, 8,14> addressHi {&data};
      BitRange<19,15,15> latch     {&data};
      BitRange<19,16,18> fineX     {&data};
    } v, t;

    n01 nmiHold;
    n01 nmiFlag;

    //$2000
    n06 vramIncrement;  //1 or 32
    n16 spriteAddress;  //0x0000 or 0x1000
    n16 bgAddress;      //0x0000 or 0x1000
    n05 spriteHeight;   //8 or 16
    n01 masterSelect;
    n01 nmiEnable;

    //$2001
    n01 grayscale;
    n01 bgEdgeEnable;
    n01 spriteEdgeEnable;
    n01 bgEnable;
    n01 spriteEnable;
    n03 emphasis;

    //$2002
    n01 spriteOverflow;
    n01 spriteZeroHit;

    //$2003
    n08 oamAddress;
  } io;

  struct OAM {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8 id = 64;
    n8 y = 0xff;
    n8 tile = 0xff;
    n8 attr = 0xff;
    n8 x = 0xff;

    n8 tiledataLo;
    n8 tiledataHi;
  };

  struct Latches {
    n16 nametable;
    n16 attribute;
    n16 tiledataLo;
    n16 tiledataHi;

    n16 oamIterator;
    n16 oamCounter;

    OAM oam[8];   //primary
    OAM soam[8];  //secondary
  } latch;
};

extern PPU ppu;
