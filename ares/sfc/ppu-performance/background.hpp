struct Background {
  struct ID { enum : u32 { BG1, BG2, BG3, BG4 }; };
  struct Mode { enum : u32 { BPP2, BPP4, BPP8, Mode7, Inactive }; };

  const u32 id;
  Background(u32 id) : id(id) {}

  //background.cpp
  auto render() -> void;
  auto getTile(u32 hoffset, u32 voffset) -> n16;
  auto power() -> void;

  //mode7.cpp
  auto renderMode7() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n2  screenSize;
    n16 screenAddress;

    n16 tiledataAddress;
    n1  tileSize;

    n16 hoffset;
    n16 voffset;

    n1  aboveEnable;
    n1  belowEnable;
    n1  mosaicEnable;

    n8  mode;
    n8  priority[2];
  } io;

  PPU::Window::Layer window;

  friend class PPU;
};
