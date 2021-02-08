struct FDSDrive {
  auto clock() -> void;
  auto change() -> void;
  auto powerup() -> void;
  auto rewind() -> void;
  auto advance() -> void;
  auto crc(n8 data) -> void;
  auto read() -> void;
  auto write() -> void;
  auto read(n16 address, n8 data) -> n8;
  auto write(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n1  enable;
  n1  power;
  n1  changing;
  n1  ready;
  n1  scan;
  n1  rewinding;
  n1  scanning;
  n1  reading;  //0 = writing
  n1  writeCRC;
  n1  clearCRC;
  n1  irq;
  n1  pending;
  n1  available;
  n32 counter;
  n32 offset;
  n1  gap;
  n8  data;
  n1  completed;
  n16 crc16;
};
