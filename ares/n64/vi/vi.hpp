//Video Interface

struct VI : Thread {
  Node::Component node;
  Node::Screen screen;

  //vi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;
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
    uint10 coincidence = 256;
    uint12 xscale;
    uint12 xsubpixel;
    uint12 yscale;
    uint12 ysubpixel;

  //internal:
     uint9 vcounter;
     uint1 field;
  } io;

//unserialized:
  u32 output[640 * 512];
  bool refreshed;
};

extern VI vi;
