struct PPU : Thread {
  Node::Component node;
  Node::Screen screen;
  Node::Boolean overscan;
  Node::Boolean colorEmulation;
  Memory::Writable<uint8> ciram;
  Memory::Writable<uint8> cgram;
  Memory::Writable<uint8> oam;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory ciram;
      Node::Memory cgram;
      Node::Memory oam;
    } memory;
  } debugger;

  auto rate() const -> uint { return Region::PAL() ? 5 : 4; }
  auto vlines() const -> uint { return Region::PAL() ? 312 : 262; }

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto scanline() -> void;
  auto frame() -> void;
  auto refresh() -> void;

  auto power(bool reset) -> void;

  //memory.cpp
  auto readCIRAM(uint11 address) -> uint8;
  auto writeCIRAM(uint11 address, uint8 data) -> void;

  auto readCGRAM(uint5 address) -> uint8;
  auto writeCGRAM(uint5 address, uint8 data) -> void;

  auto readIO(uint16 address) -> uint8;
  auto writeIO(uint16 address, uint8 data) -> void;

  //render.cpp
  auto enable() const -> bool;
  auto loadCHR(uint16 address) -> uint8;

  auto renderPixel() -> void;
  auto renderSprite() -> void;
  auto renderScanline() -> void;

  //color.cpp
  auto color(uint32) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint32 buffer[256 * 262];

  struct IO {
    //internal
     uint8 mdr;
     uint1 field;
    uint16 lx;
    uint16 ly;
     uint8 busData;

    struct Union {
      uint19 data;
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

     uint1 nmiHold;
     uint1 nmiFlag;

    //$2000
     uint6 vramIncrement = 1;  //1 or 32
    uint16 spriteAddress;      //0x0000 or 0x1000
    uint16 bgAddress;          //0x0000 or 0x1000
     uint5 spriteHeight = 8;   //8 or 16
     uint1 masterSelect;
     uint1 nmiEnable;

    //$2001
     uint1 grayscale;
     uint1 bgEdgeEnable;
     uint1 spriteEdgeEnable;
     uint1 bgEnable;
     uint1 spriteEnable;
     uint3 emphasis;

    //$2002
     uint1 spriteOverflow;
     uint1 spriteZeroHit;

    //$2003
     uint8 oamAddress;
  } io;

  struct OAM {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint8 id = 64;
    uint8 y = 0xff;
    uint8 tile = 0xff;
    uint8 attr = 0xff;
    uint8 x = 0xff;

    uint8 tiledataLo;
    uint8 tiledataHi;
  };

  struct Latches {
    uint16 nametable;
    uint16 attribute;
    uint16 tiledataLo;
    uint16 tiledataHi;

    uint16 oamIterator;
    uint16 oamCounter;

    OAM oam[8];   //primary
    OAM soam[8];  //secondary
  } latch;
};

extern PPU ppu;
