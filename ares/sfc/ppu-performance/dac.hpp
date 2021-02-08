struct DAC {
  struct Pixel;

  //dac.cpp
  auto prepare() -> void;
  auto render() -> void;
  auto pixel(n8 x, Pixel above, Pixel below) const -> n15;
  auto blend(n15 x, n15 y, bool halve) const -> n15;
  auto plotAbove(n8 x, n8 source, n8 priority, n15 color) -> void;
  auto plotBelow(n8 x, n8 source, n8 priority, n15 color) -> void;
  auto directColor(n8 palette, n3 paletteGroup) const -> n15;
  auto fixedColor() const -> n15;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n15 cgram[256];

  struct IO {
    n1 directColor;
    n1 blendMode;
    n1 colorEnable[7];
    n1 colorHalve;
    n1 colorMode;
    n5 colorRed;
    n5 colorGreen;
    n5 colorBlue;
  } io;

  PPU::Window::Color window;

//unserialized:
  struct Pixel {
    n8  source;
    n8  priority;
    n15 color;
  } above[256], below[256];

  bool windowAbove[256];
  bool windowBelow[256];

  friend class PPU;
};
