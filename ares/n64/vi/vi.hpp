//Video Interface

struct VI : Thread, Memory::IO<VI> {
  Node::Component node;
  Node::Screen screen;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Notification io;
    } tracer;
  } debugger;

  //vi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;
  auto power() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
     uint2 colorDepth = 0;
     uint1 gammaDither = 0;
     uint1 gamma = 0;
     uint1 divot = 0;
     uint1 serrate = 0;
     uint2 antialias = 0;
    uint32 reserved = 0;
    uint24 dramAddress = 0;
    uint12 width = 0;
    uint10 coincidence = 256;
     uint8 hsyncWidth = 0;
     uint8 colorBurstWidth = 0;
     uint4 vsyncWidth = 0;
    uint10 colorBurstHsync = 0;
    uint10 halfLinesPerField = 0;
    uint12 quarterLineDuration = 0;
     uint5 palLeapPattern = 0;
    uint12 hsyncLeap[2] = {};
    uint10 hend = 0;
    uint10 hstart = 0;
    uint10 vend = 0;
    uint10 vstart = 0;
    uint10 colorBurstEnd = 0;
    uint10 colorBurstStart = 0;
    uint12 xscale = 0;
    uint12 xsubpixel = 0;
    uint12 yscale = 0;
    uint12 ysubpixel = 0;

  //internal:
     uint9 vcounter = 0;
     uint1 field = 0;
  } io;

//unserialized:
  u32 output[640 * 512];
  bool refreshed;
};

extern VI vi;
