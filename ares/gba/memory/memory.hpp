struct IO {
  virtual auto readIO(n32 address) -> n8 = 0;
  virtual auto writeIO(n32 address, n8 data) -> void = 0;
  auto readIO(u32 mode, n32 address) -> n32;
  auto writeIO(u32 mode, n32 address, n32 word) -> void;
};

struct Bus {
  auto power() -> void;

  IO* io[0x400] = {nullptr};
};

extern Bus bus;
