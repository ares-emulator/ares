//Peripheral Interface

struct PI {
  //pi.cpp
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    u32 dramAddress;
    u32 cartAddress;
    u32 readLength;
    u32 writeLength;
  } io;
};

extern PI pi;
