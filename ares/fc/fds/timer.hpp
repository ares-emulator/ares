struct FDSTimer {
  auto clock() -> void;
  auto read(uint16 address, uint8 data) -> uint8;
  auto write(uint16 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

   uint1 enable;
  uint16 counter;
  uint16 period;
   uint1 repeat;
   uint1 irq;
   uint1 pending;
};
