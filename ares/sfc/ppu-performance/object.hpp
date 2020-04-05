struct Object {
  //object.cpp
  auto addressReset() -> void;
  auto setFirstSprite() -> void;
  auto render() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  OAM oam;

  struct IO {
     uint1 interlace;

    uint16 tiledataAddress;
     uint2 nameselect;
     uint3 baseSize;
     uint7 firstSprite;

     uint1 aboveEnable;
     uint1 belowEnable;

     uint1 rangeOver;
     uint1 timeOver;

     uint8 priority[4];
  } io;

  PPU::Window::Layer window;

  //unserialized:
  struct Item {
     uint1 valid;
     uint7 index;
     uint8 width;
     uint8 height;
  } items[32];

  struct Tile {
     uint1 valid;
     uint9 x;
     uint2 priority;
     uint8 palette;
     uint1 hflip;
    uint32 data;
  } tiles[34];

  friend class PPU;
};
