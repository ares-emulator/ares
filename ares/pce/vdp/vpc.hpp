//Hudson Soft HuC6202: Video Priority Controller

struct VPC {
  //vpc.cpp
  auto bus(uint16 hcounter) const -> uint9;

  auto read(uint5 address) -> uint8;
  auto write(uint5 address, uint8 data) -> void;
  auto store(uint2 address, uint8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Settings {
    uint1 enableVDC0 = 1;
    uint1 enableVDC1 = 0;
    uint2 priority = 0;
  } settings[4];

  uint10 window[2];
   uint1 select;
};
