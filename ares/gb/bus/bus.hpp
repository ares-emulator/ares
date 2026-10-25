struct Bus {
  //megaduck.cpp
  auto megaDuckAddress(n16 address) -> maybe<n16>;
  auto megaDuckReadData(n16 address, n8 data) -> n8;
  auto megaDuckWriteData(n16 address, n8 data) -> n8;

  auto read(u32 cycle, n16 address, n8 data) -> n8;
  auto write(u32 cycle, n16 address, n8 data) -> void;

  auto read(n16 address, n8 data) -> n8;
  auto write(n16 address, n8 data) -> void;
};

extern Bus bus;
