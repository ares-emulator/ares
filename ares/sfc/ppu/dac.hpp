struct DAC {
  auto scanline() -> void;
  auto run() -> void;
  auto power() -> void;

  auto below(bool hires) -> uint16;
  auto above() -> uint16;

  auto blend(uint x, uint y) const -> uint15;
  auto paletteColor(uint8 palette) const -> uint15;
  auto directColor(uint8 palette, uint3 paletteGroup) const -> uint15;
  auto fixedColor() const -> uint15;

  auto serialize(serializer&) -> void;

  uint32* lineA;
  uint32* lineB;

  uint15 cgram[256];

  struct IO {
    uint1 directColor;
    uint1 blendMode;

    struct Layer {
      uint1 colorEnable;
    } bg1, bg2, bg3, bg4, obj, back;
    uint1 colorHalve;
    uint1 colorMode;

    uint5 colorRed;
    uint5 colorGreen;
    uint5 colorBlue;
  } io;

  struct Math {
    struct Screen {
      uint15 color;
       uint1 colorEnable;
    } above, below;
    uint1 transparent;
    uint1 blendMode;
    uint1 colorHalve;
  } math;

  friend class PPU;
};
