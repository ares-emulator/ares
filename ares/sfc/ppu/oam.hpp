struct OAM {
  auto read(n10 address) -> n8;
  auto write(n10 address, n8 data) -> void;

  struct Object {
    auto width() const -> uint;
    auto height() const -> uint;

    n9 x;
    n8 y;
    n8 character;
    n1 nameselect;
    n1 vflip;
    n1 hflip;
    n2 priority;
    n3 palette;
    n1 size;
  } object[128];
};
