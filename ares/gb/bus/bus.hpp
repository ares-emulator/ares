struct Bus {
  auto read(uint cycle, uint16 address, uint8 data) -> uint8;
  auto write(uint cycle, uint16 address, uint8 data) -> void;

  auto read(uint16 address, uint8 data) -> uint8;
  auto write(uint16 address, uint8 data) -> void;
};

extern Bus bus;
