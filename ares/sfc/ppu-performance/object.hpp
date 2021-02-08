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
    n1  interlace;

    n16 tiledataAddress;
    n2  nameselect;
    n3  baseSize;
    n7  firstSprite;

    n1  aboveEnable;
    n1  belowEnable;

    n1  rangeOver;
    n1  timeOver;

    n8  priority[4];
  } io;

  PPU::Window::Layer window;

  //unserialized:
  struct Item {
    n1  valid;
    n7  index;
    n8  width;
    n8  height;
  } items[32];

  struct Tile {
    n1  valid;
    n9  x;
    n2  priority;
    n8  palette;
    n1  hflip;
    n32 data;
  } tiles[34];

  friend class PPU;
};
