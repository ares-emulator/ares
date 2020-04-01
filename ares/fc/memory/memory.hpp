struct Bus {
  auto read(uint16 address) -> uint8;
  auto write(uint16 address, uint8 data) -> void;
};

extern Bus bus;
