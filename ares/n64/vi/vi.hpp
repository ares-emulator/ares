//Video Interface

struct VI {
  //vi.cpp
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
     uint2 colorDepth;
    uint32 dramAddress;
    uint12 width;
  } io;
};

extern VI vi;
