struct Background {
  struct ID { enum : u32 { BG1, BG2, BG3, BG4 }; };
  const u32 id;
  Background(u32 id) : id(id) {}

  //background.cpp
  auto hires() const -> bool;
  auto frame() -> void;
  auto scanline() -> void;
  auto begin() -> void;
  auto fetchNameTable() -> void;
  auto fetchOffset(u32 y) -> void;
  auto fetchCharacter(u32 index, bool half = 0) -> void;
  auto run(bool screen) -> void;
  auto power() -> void;

  //mode7.cpp
  auto runMode7() -> void;

  auto serialize(serializer&) -> void;

  struct Mode { enum : u32 { BPP2, BPP4, BPP8, Mode7, Inactive }; };
  struct ScreenSize { enum : u32 { Size32x32, Size32x64, Size64x32, Size64x64 }; };
  struct TileSize { enum : u32 { Size8x8, Size16x16 }; };
  struct Screen { enum : u32 { Above, Below }; };

  struct IO {
    n2  screenSize;
    n16 screenAddress;

    n16 tiledataAddress;
    n1  tileSize;

    n8  mode;
    n8  priority[2];

    n1  aboveEnable;
    n1  belowEnable;

    n16 hoffset;
    n16 voffset;
  } io;

  struct Pixel {
    n8 priority;  //0 = none (transparent)
    n8 palette;
    n3 paletteGroup;
  } above, below;

  struct Output {
    Pixel above;
    Pixel below;
  } output;

  struct Mosaic {
    n1  enable;
    n16 hcounter;
    n16 hoffset;
    Pixel pixel;
  } mosaic;

  struct OffsetPerTile {
    //set in BG3 only; used by BG1 and BG2
    n16 hoffset;
    n16 voffset;
  } opt;

  struct Tile {
    n16 address;
    n10 character;
    n8  palette;
    n3  paletteGroup;
    n8  priority;
    n1  hmirror;
    n1  vmirror;
    n16 data[4];
  } tiles[66];

  n7 renderingIndex;
  n3 pixelCounter;

  friend class PPU;
};
