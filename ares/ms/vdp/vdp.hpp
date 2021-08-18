//Video Display Processor
//315-5124 (SMS1)
//315-5246 (SMS2/GG)

struct VDP : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Natural revision;
  Node::Setting::Boolean interframeBlending;  //Game Gear only
  Memory::Writable<n8 > vram;  //16KB
  Memory::Writable<n12> cram;  //Master System = 6-bit; Game Gear = 12-bit

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(n4 register, n8 data) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory cram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  auto displayEnable() const -> n1 { return io.displayEnable; }
  auto videoMode() const -> n4 { return io.videoMode; }
  auto vcounter() const -> u32 { return io.vcounter; }
  auto hcounter() const -> u32 { return io.hcounter; }

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto vlines() -> u32;
  auto vblank() -> bool;

  auto power() -> void;

  //io.cpp
  auto vcounterQuery() -> n8;
  auto hcounterQuery() -> n8;
  auto hcounterLatch() -> void;
  auto ccounter() -> n12;
  auto data() -> n8;
  auto status() -> n8;

  auto data(n8) -> void;
  auto control(n8) -> void;
  auto registerWrite(n4 addr, n8 data) -> void;

  struct Background {
    VDP& self;

    //background.cpp
    auto setup(n9 voffset) -> void;
    auto run(n8 hoffset, n9 voffset) -> void;
    auto graphics1(n8 hoffset, n9 voffset) -> void;
    auto graphics2(n8 hoffset, n9 voffset) -> void;
    auto graphics3(n8 hoffset, n9 voffset, u32 vlines) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1 hscrollLock;
      n1 vscrollLock;
      n4 nameTableAddress;
      n8 colorTableAddress;
      n3 patternTableAddress;
      n8 hscroll;
      n8 vscroll;
    } io;

    struct Latch {
      n4 nameTableAddress;
      n8 hscroll;
      n8 vscroll;
    } latch;

    struct Output {
      n4 color;
      n1 palette;
      n1 priority;
    } output;
  } background{*this};

  struct Sprite {
    VDP& self;

    //sprite.cpp
    auto setup(n9 voffset) -> void;
    auto run(n8 hoffset, n9 voffset) -> void;
    auto graphics1(n8 hoffset, n9 voffset) -> void;
    auto graphics2(n8 hoffset, n9 voffset) -> void;
    auto graphics3(n8 hoffset, n9 voffset, u32 vlines) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Object {
      i9 x;
      i9 y = 0xd0;
      n8 pattern;
      n4 color;
    } objects[8];

    struct IO {
      n1 zoom;
      n1 size;
      n1 shift;
      n7 attributeTableAddress;
      n3 patternTableAddress;

      //flags
      n5 overflowIndex = 0b11111;
      n1 overflow;
      n1 collision;
    } io;

    struct Output {
      n4 color;
    } output;
  } sprite{*this};

  struct DAC {
    VDP& self;

    //dac.cpp
    auto setup(n8 voffset) -> void;
    auto run(n8 hoffset, n8 voffset) -> void;
    auto palette(n5 index) -> n12;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1 externalSync;  //todo
      n1 leftClip;
      n4 backdropColor;
    } io;

  //unserialized:
    u32* output = nullptr;
  } dac{*this};

  //color.cpp
  auto colorMasterSystem(n32) -> n64;
  auto colorGameGear(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct IRQ {
    VDP& self;

    //irq.cpp
    auto poll() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Line {
      n1 enable;
      n1 pending;
      n8 counter;
      n8 coincidence = 0xff;
    } line;

    struct Frame {
      n1 enable;
      n1 pending;
    } frame;
  } irq{*this};

  struct IO {
    n2  code;
    n14 address;

    n1  displayEnable;
    n4  videoMode;

    //counters
    u32 vcounter = 0;  //vertical counter
    u32 hcounter = 0;  //horizontal counter
    n12 ccounter = 0;  //csync counter
  } io;

  struct Latch {
    n1 control;
    n8 hcounter;
    n8 vram;
    n8 cram;  //Game Gear only
  } latch;
};

extern VDP vdp;
