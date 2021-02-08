struct OAM {
  //oam.cpp
  auto read(n10 address) -> n8;
  auto write(n10 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Object {
    //oam.cpp
    auto width() const -> u32;
    auto height() const -> u32;

    n9 x;
    n8 y;
    n8 character;
    n1 nameselect;
    n1 vflip;
    n1 hflip;
    n2 priority;
    n3 palette;
    n1 size;
  } objects[128];
};
