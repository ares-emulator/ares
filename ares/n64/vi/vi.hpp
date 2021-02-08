//Video Interface

struct VI : Thread, Memory::IO<VI> {
  Node::Object node;
  Node::Video::Screen screen;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //vi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto refresh() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n2  colorDepth = 0;
    n1  gammaDither = 0;
    n1  gamma = 0;
    n1  divot = 0;
    n1  serrate = 0;
    n2  antialias = 0;
    n32 reserved = 0;
    n24 dramAddress = 0;
    n12 width = 0;
    n10 coincidence = 256;
    n8  hsyncWidth = 0;
    n8  colorBurstWidth = 0;
    n4  vsyncWidth = 0;
    n10 colorBurstHsync = 0;
    n10 halfLinesPerField = 0;
    n12 quarterLineDuration = 0;
    n5  palLeapPattern = 0;
    n12 hsyncLeap[2] = {};
    n10 hend = 0;
    n10 hstart = 0;
    n10 vend = 0;
    n10 vstart = 0;
    n10 colorBurstEnd = 0;
    n10 colorBurstStart = 0;
    n12 xscale = 0;
    n12 xsubpixel = 0;
    n12 yscale = 0;
    n12 ysubpixel = 0;

  //internal:
    n9  vcounter = 0;
    n1  field = 0;
  } io;

//unserialized:
  bool refreshed;
};

extern VI vi;
