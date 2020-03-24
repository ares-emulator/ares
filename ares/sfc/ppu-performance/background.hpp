struct Background {
  struct ID { enum : uint { BG1, BG2, BG3, BG4 }; };
  struct Mode { enum : uint { BPP2, BPP4, BPP8, Mode7, Inactive }; };

  const uint id;
  Background(uint id) : id(id) {}

  //background.cpp
  auto render() -> void;
  auto getTile(uint hoffset, uint voffset) -> uint16;
  auto power() -> void;

  //mode7.cpp
  auto renderMode7() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
     uint2 screenSize;
    uint16 screenAddress;

    uint16 tiledataAddress;
     uint1 tileSize;

    uint16 hoffset;
    uint16 voffset;

     uint1 aboveEnable;
     uint1 belowEnable;
     uint1 mosaicEnable;

     uint8 mode;
     uint8 priority[2];
  } io;

  PPU::Window::Layer window;

  friend class PPU;
};
