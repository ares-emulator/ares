//Graphics Processing Unit

struct GPU : Thread {
  Node::Component node;
  Node::Screen screen;

  //gpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readByte(u32 address) -> u8;
  auto readHalf(u32 address) -> u16;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    uint16 vcounter = 0;
     uint2 dmaDirection = 0;
  } io;

//unserialized:
  u32 output[320 * 240];
  bool refreshed;
};

extern GPU gpu;
