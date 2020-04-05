struct OAM {
  //oam.cpp
  auto read(uint10 address) -> uint8;
  auto write(uint10 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Object {
    //oam.cpp
    auto width() const -> uint;
    auto height() const -> uint;

    uint9 x;
    uint8 y;
    uint8 character;
    uint1 nameselect;
    uint1 vflip;
    uint1 hflip;
    uint2 priority;
    uint3 palette;
    uint1 size;
  } objects[128];
};
