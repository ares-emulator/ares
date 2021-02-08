struct DAC {
  auto scanline() -> void;
  auto run() -> void;
  auto power() -> void;

  auto below(bool hires) -> n16;
  auto above() -> n16;

  auto blend(u32 x, u32 y) const -> n15;
  auto paletteColor(n8 palette) const -> n15;
  auto directColor(n8 palette, n3 paletteGroup) const -> n15;
  auto fixedColor() const -> n15;

  auto serialize(serializer&) -> void;

  n15 cgram[256];

  struct IO {
    n1 directColor;
    n1 blendMode;

    struct Layer {
      n1 colorEnable;
    } bg1, bg2, bg3, bg4, obj, back;
    n1 colorHalve;
    n1 colorMode;

    n5 colorRed;
    n5 colorGreen;
    n5 colorBlue;
  } io;

  struct Math {
    struct Screen {
      n15 color;
      n1  colorEnable;
    } above, below;
    n1 transparent;
    n1 blendMode;
    n1 colorHalve;
  } math;

  friend class PPU;

//unserialized:
  u32* line = nullptr;
};
