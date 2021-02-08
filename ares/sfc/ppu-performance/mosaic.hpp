struct Mosaic {
  //mosaic.cpp
  auto enable() const -> bool;
  auto voffset() const -> u32;
  auto scanline() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n5 size;
  n5 vcounter;
};
