struct DIP {
  //dip.cpp
  auto power() -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8 value = 0x03; // default game time used for the competition is 6 minutes
};

extern DIP dip;
