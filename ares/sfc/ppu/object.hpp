struct Object {
  auto addressReset() -> void;
  auto setFirstSprite() -> void;
  auto frame() -> void;
  auto scanline() -> void;
  auto evaluate(uint7 index) -> void;
  auto run() -> void;
  auto fetch() -> void;
  auto power() -> void;

  auto onScanline(PPU::OAM::Object&) -> bool;

  auto serialize(serializer&) -> void;

  OAM oam;

  struct IO {
     uint1 aboveEnable;
     uint1 belowEnable;
     uint1 interlace;

    uint16 tiledataAddress;
     uint2 nameselect;
     uint3 baseSize;
     uint7 firstSprite;

     uint8 priority[4];

     uint1 rangeOver;
     uint1 timeOver;
  } io;

  struct Latch {
     uint7 firstSprite;
  } latch;

  struct Item {
     uint1 valid;
     uint7 index;
  };

  struct Tile {
     uint1 valid;
     uint9 x;
     uint2 priority;
     uint8 palette;
     uint1 hflip;
    uint32 data;
  };

  struct State {
    uint x;
    uint y;

    uint itemCount;
    uint tileCount;

    bool active;
    Item item[2][32];
    Tile tile[2][34];
  } t;

  struct Output {
    struct Pixel {
      uint8 priority;  //0 = none (transparent)
      uint8 palette;
    } above, below;
  } output;

  friend class PPU;
};
