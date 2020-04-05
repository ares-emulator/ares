struct DAC {
  struct Pixel;

  //dac.cpp
  auto prepare() -> void;
  auto render() -> void;
  auto pixel(uint8 x, Pixel above, Pixel below) const -> uint15;
  auto blend(uint15 x, uint15 y, bool halve) const -> uint15;
  auto plotAbove(uint8 x, uint8 source, uint8 priority, uint15 color) -> void;
  auto plotBelow(uint8 x, uint8 source, uint8 priority, uint15 color) -> void;
  auto directColor(uint8 palette, uint3 paletteGroup) const -> uint15;
  auto fixedColor() const -> uint15;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint15 cgram[256];

  struct IO {
    uint1 directColor;
    uint1 blendMode;
    uint1 colorEnable[7];
    uint1 colorHalve;
    uint1 colorMode;
    uint5 colorRed;
    uint5 colorGreen;
    uint5 colorBlue;
  } io;

  PPU::Window::Color window;

//unserialized:
  struct Pixel {
     uint8 source;
     uint8 priority;
    uint15 color;
  } above[256], below[256];

  bool windowAbove[256];
  bool windowBelow[256];

  friend class PPU;
};
