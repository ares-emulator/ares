struct Window {
  struct Layer;
  struct Color;

  //window.cpp
  auto render(Layer&, bool enable, bool output[256]) -> void;
  auto render(Color&, u32 mask, bool output[256]) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Layer {
    n1 oneInvert;
    n1 oneEnable;
    n1 twoInvert;
    n1 twoEnable;
    n2 mask;
    n1 aboveEnable;
    n1 belowEnable;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  };

  struct Color {
    n1 oneInvert;
    n1 oneEnable;
    n1 twoInvert;
    n1 twoEnable;
    n2 mask;
    n2 aboveMask;
    n2 belowMask;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  };

  struct IO {
    //$2126  WH0
    n8 oneLeft;

    //$2127  WH1
    n8 oneRight;

    //$2128  WH2
    n8 twoLeft;

    //$2129  WH3
    n8 twoRight;
  } io;

  friend class PPU;
};
