struct Mosaic {
  //mosaic.cpp
  auto enable() const -> bool;
  auto voffset() const -> uint;
  auto scanline() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint5 size;
  uint5 vcounter;
};
