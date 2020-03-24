struct Window {
  //window.cpp
  auto scanline() -> void;
  auto run() -> void;
  auto test(bool oneEnable, bool one, bool twoEnable, bool two, uint mask) -> bool;
  auto power() -> void;

  auto serialize(serializer&) -> void;

  struct IO {
    struct Layer {
      uint1 oneInvert;
      uint1 oneEnable;
      uint1 twoInvert;
      uint1 twoEnable;
      uint2 mask;
      uint1 aboveEnable;
      uint1 belowEnable;
    } bg1, bg2, bg3, bg4, obj;

    struct Color {
      uint1 oneEnable;
      uint1 oneInvert;
      uint1 twoEnable;
      uint1 twoInvert;
      uint2 mask;
      uint2 aboveMask;
      uint2 belowMask;
    } col;

    uint8 oneLeft;
    uint8 oneRight;
    uint8 twoLeft;
    uint8 twoRight;
  } io;

  struct Output {
    struct Pixel {
      uint1 colorEnable;
    } above, below;
  } output;

  struct {
    uint x;
  };

  friend class PPU;
};
