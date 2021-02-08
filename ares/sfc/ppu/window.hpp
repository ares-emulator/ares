struct Window {
  //window.cpp
  auto scanline() -> void;
  auto run() -> void;
  auto test(bool oneEnable, bool one, bool twoEnable, bool two, u32 mask) -> bool;
  auto power() -> void;

  auto serialize(serializer&) -> void;

  struct IO {
    struct Layer {
      n1 oneInvert;
      n1 oneEnable;
      n1 twoInvert;
      n1 twoEnable;
      n2 mask;
      n1 aboveEnable;
      n1 belowEnable;
    } bg1, bg2, bg3, bg4, obj;

    struct Color {
      n1 oneEnable;
      n1 oneInvert;
      n1 twoEnable;
      n1 twoInvert;
      n2 mask;
      n2 aboveMask;
      n2 belowMask;
    } col;

    n8 oneLeft;
    n8 oneRight;
    n8 twoLeft;
    n8 twoRight;
  } io;

  struct Output {
    struct Pixel {
      n1 colorEnable;
    } above, below;
  } output;

  struct {
    u32 x;
  };

  friend class PPU;
};
