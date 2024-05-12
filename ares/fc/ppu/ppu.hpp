struct PPU : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Memory::Writable<n8> ciram;
  Memory::Writable<n8> cgram;

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
    n8  mdr;
    n1  field;
    n16 lx;
    n16 ly;
    n8  busData;

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

    auto operator[](n2 timing) -> n8& {
      switch(timing) {
      case 0: return y;
      case 1: return tile;
      case 2: return attr;
      case 3: return x;
      }
      return y;
    }

    n8 tiledataLo;
    n8 tiledataHi;
  };

  struct Latches {
    n16 nametable;
    n16 attribute;
    n16 tiledataLo;
    n16 tiledataHi;

    OAM oam[8];   //primary
  } latch;

  struct SpriteEvaluation {
    Memory::Writable<n8> oam;

    // sprite.cpp
    auto load() -> void;
    auto unload() -> void;

    auto main() -> void;
    auto power(bool reset) -> void;

    // memory.cpp
    auto oamData() -> n8 const;
    auto oamData(n8 data) -> void;

    // serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
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
    } io;

    OAM soam[8]; // secondary oam
  } spriteEvaluation;

  u32* output;
};

extern PPU ppu;
