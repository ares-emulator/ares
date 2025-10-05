struct PPU : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Memory::Writable<n8> ciram;
  Memory::Writable<n6> cgram;
  Memory::Writable<n8> oam;
  Memory::Writable<n8> soam;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    struct Memory {
      Node::Debugger::Memory ciram;
      Node::Debugger::Memory cgram;
      Node::Debugger::Memory oam;
      Node::Debugger::Memory soam;
    } memory;
  } debugger;

  auto rate() const -> u32 { return Region::PAL() ? 5 : 4; }
  auto vlines() const -> u32 { return Region::PAL() ? 312 : 262; }
  auto displayHeight() const -> u32 { return Region::PAL() ? 288 : 242; }

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

  auto readCGRAM(n5 address) -> n6;
  auto writeCGRAM(n5 address, n6 data) -> void;

  auto readIO(n16 address) -> n8;
  auto writeIO(n16 address, n8 data) -> void;

  //render.cpp
  auto enable() const -> bool;
  auto rendering() const -> bool;
  auto loadCHR(n16 address) -> n8;

  auto renderPixel() -> void;
  auto renderScanline() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  // scroll.cpp
  auto incrementVRAMAddressX() -> void;
  auto incrementVRAMAddressY() -> void;
  auto transferScrollX() -> void;
  auto transferScrollY() -> void;
  auto cycleScroll() -> void;
  auto scrollTransferDelay() -> void;

  // sprite.cpp
  auto cycleSpriteEvaluation() -> void;
  auto cyclePrepareSpriteEvaluation() -> void;

  struct ScrollRegisters {
    n15 data;

    BitRange<15, 0, 4> tileX     {&data};
    BitRange<15, 5, 9> tileY     {&data};
    BitRange<15,10,10> nametableX{&data};
    BitRange<15,11,11> nametableY{&data};
    BitRange<15,12,14> fineY     {&data};
    n1 latch;
    n3 fineX;

    BitRange<15,10,11> nametable {&data};
    BitRange<15, 0,14> address   {&data};
    BitRange<15, 0, 7> addressLo {&data};
    BitRange<15, 8,14> addressHi {&data};

    n8 transferDelay;
  } scroll;

  struct VRAMAddressRegisters {
    n15 data;

    BitRange<15, 0, 4> tileX     {&data};
    BitRange<15, 5, 9> tileY     {&data};
    BitRange<15,10,10> nametableX{&data};
    BitRange<15,11,11> nametableY{&data};
    BitRange<15,12,14> fineY     {&data};

    BitRange<15,10,11> nametable {&data};
    BitRange<15, 0,14> address   {&data};

    BitRange<15, 2, 4> attrX     {&data};
    BitRange<15, 7, 9> attrY     {&data};

    n8 latchData;
    n8 blockingRead;
  } var;

  struct IO {
    n14 busAddress;

    //internal
    n8  mdr;
    n1  field;
    n16 lx;
    n16 ly;

    n1  nmiHold;
    n1  nmiFlag;

    //$2000
    n6  vramIncrement = 1;  //1 or 32
    n16 spriteAddress;      //0x0000 or 0x1000
    n16 bgAddress;          //0x0000 or 0x1000
    n5  spriteHeight = 8;   //8 or 16
    n1  masterSelect;
    n1  nmiEnable;

    //$2001
    n1  grayscale;
    n1  bgEdgeEnable;
    n1  spriteEdgeEnable;
    n1  bgEnable;
    n1  spriteEnable;
    n3  emphasis;

    //$2002
    n1  spriteZeroHit;
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

    n8  oamId[8];
    OAM oam[8];   //primary
  } latch;

  struct SpriteEvaluation {
    // $2002 bit5
    n1 spriteOverflow;

    // $2003
    n8 oamAddress;

    // $2004
    n8 oamData;

    // main oam counter (oamAddress)
    BitRange<8,0,7> oamMainCounter{&oamAddress};
    bool oamMainCounterOverflow;
    // every sprite has 4 bytes
    BitRange<8,0,1> oamMainCounterTiming{&oamAddress};
    // main counter have 64 sprites
    BitRange<8,2,7> oamMainCounterIndex{&oamAddress};

    // secondary oam counter
    n5  oamTempCounter;
    bool oamTempCounterOverflow;
    // every sprite has 4 bytes
    BitRange<5,0,1> oamTempCounterTiming{&oamTempCounter};
    // temp counter have 8 sprites
    BitRange<5,2,4> oamTempCounterIndex{&oamTempCounter};
  } sprite;

  u32* output;

  static constexpr u8 cgramBootValue[32] = {
    0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D,
    0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
    0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14,
    0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08,
  };
};

extern PPU ppu;
