//Hudson Soft HuC6202: Video Priority Controller

struct VPCBase {
  virtual auto read(n5 address) -> n8 = 0;
  virtual auto write(n5 address, n8 data) -> void = 0;
  virtual auto store(n2 address, n8 data) -> void = 0;
};

struct VPC : VPCBase {
  //vpc.cpp
  auto bus(n16 hcounter) const -> n9;

  auto read(n5 address) -> n8 override;
  auto write(n5 address, n8 data) -> void override;
  auto store(n2 address, n8 data) -> void override;
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
