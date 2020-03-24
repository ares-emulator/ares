struct Mosaic {
  //mosaic.cpp
  inline auto enable() const -> bool;
  inline auto voffset() const -> uint;
  auto scanline() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint5 size;
  uint5 vcounter;
};
