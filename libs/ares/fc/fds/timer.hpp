struct FDSTimer {
  auto clock() -> void;
  auto read(n16 address, n8 data) -> n8;
  auto write(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n1  enable;
  n16 counter;
  n16 period;
  n1  repeat;
  n1  irq;
  n1  pending;
};
