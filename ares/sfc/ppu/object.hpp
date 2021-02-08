struct Object {
  auto addressReset() -> void;
  auto setFirstSprite() -> void;
  auto frame() -> void;
  auto scanline() -> void;
  auto evaluate(n7 index) -> void;
  auto run() -> void;
  auto fetch() -> void;
  auto power() -> void;

  auto onScanline(PPU::OAM::Object&) -> bool;

  auto serialize(serializer&) -> void;

  OAM oam;

  struct IO {
    n1  aboveEnable;
    n1  belowEnable;
    n1  interlace;

    n16 tiledataAddress;
    n2  nameselect;
    n3  baseSize;
    n7  firstSprite;

    n8  priority[4];

    n1  rangeOver;
    n1  timeOver;
  } io;

  struct Latch {
    n7  firstSprite;
  } latch;

  struct Item {
    n1  valid;
    n7  index;
  };

  struct Tile {
    n1  valid;
    n9  x;
    n2  priority;
    n8  palette;
    n1  hflip;
    n32 data;
  };

  struct State {
    u32  x;
    u32  y;

    u32  itemCount;
    u32  tileCount;

    bool active;
    Item item[2][32];
    Tile tile[2][34];
  } t;

  struct Output {
    struct Pixel {
      n8 priority;  //0 = none (transparent)
      n8 palette;
    } above, below;
  } output;

  friend class PPU;
};
