struct FDSTimer {
  auto clock() -> void;
  auto read(n16 address, n8 data) -> n8;
  auto write(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n01 enable;
  n16 counter;
  n16 period;
  n01 repeat;
  n01 irq;
  n01 pending;
};
