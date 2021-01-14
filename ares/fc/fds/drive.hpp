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

  n01 enable;
  n01 power;
  n01 changing;
  n01 ready;
  n01 scan;
  n01 rewinding;
  n01 scanning;
  n01 reading;  //0 = writing
  n01 writeCRC;
  n01 clearCRC;
  n01 irq;
  n01 pending;
  n01 available;
  n32 counter;
  n32 offset;
  n01 gap;
  n08 data;
  n01 completed;
  n16 crc16;
};
