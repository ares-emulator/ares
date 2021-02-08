//Hudson Soft HuC6202: Video Priority Controller

struct VPC {
  //vpc.cpp
  auto bus(n16 hcounter) const -> n9;

  auto read(n5 address) -> n8;
  auto write(n5 address, n8 data) -> void;
  auto store(n2 address, n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Settings {
    n1 enableVDC0 = 1;
    n1 enableVDC1 = 0;
    n2 priority = 0;
  } settings[4];

  n10 window[2];
  n1  select;
};
