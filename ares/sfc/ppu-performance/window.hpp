struct Window {
  struct Layer;
  struct Color;

  //window.cpp
  auto render(Layer&, bool enable, bool output[256]) -> void;
  auto render(Color&, uint mask, bool output[256]) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Layer {
    uint1 oneInvert;
    uint1 oneEnable;
    uint1 twoInvert;
    uint1 twoEnable;
    uint2 mask;
    uint1 aboveEnable;
    uint1 belowEnable;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  };

  struct Color {
    uint1 oneInvert;
    uint1 oneEnable;
    uint1 twoInvert;
    uint1 twoEnable;
    uint2 mask;
    uint2 aboveMask;
    uint2 belowMask;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  };

  struct IO {
    //$2126  WH0
    uint8 oneLeft;

    //$2127  WH1
    uint8 oneRight;

    //$2128  WH2
    uint8 twoLeft;

    //$2129  WH3
    uint8 twoRight;
  } io;

  friend class PPU;
};
