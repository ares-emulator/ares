struct FDSDrive {
  auto clock() -> void;
  auto change() -> void;
  auto powerup() -> void;
  auto rewind() -> void;
  auto advance() -> void;
  auto crc(uint8 data) -> void;
  auto read() -> void;
  auto write() -> void;
  auto read(uint16 address, uint8 data) -> uint8;
  auto write(uint16 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

   uint1 enable;
   uint1 power;
   uint1 changing;
   uint1 ready;
   uint1 scan;
   uint1 rewinding;
   uint1 scanning;
   uint1 reading;  //0 = writing
   uint1 writeCRC;
   uint1 clearCRC;
   uint1 irq;
   uint1 pending;
   uint1 available;
  uint32 counter;
  uint32 offset;
   uint1 gap;
   uint8 data;
   uint1 completed;
  uint16 crc16;
};
