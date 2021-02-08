struct DIP {
  //dip.cpp
  auto power() -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8 value = 0x00;
};

extern DIP dip;
