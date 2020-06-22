//Graphics Processing Unit

struct GPU : Thread, Memory::IO<GPU> {
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
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    u16 vcounter = 0;
  } io;

//unserialized:
  u32 output[320 * 240];
  bool refreshed;
};

extern GPU gpu;
