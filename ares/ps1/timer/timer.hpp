struct Timer {
  Node::Component node;

  //timer.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(uint clocks) -> void;
  auto hsync(bool line) -> void;
  auto vsync(bool line) -> void;
  auto poll() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Counter {
    u32 dotclock = 0;
    u32 divclock = 0;
  } counter;

  struct Source {
    Timer& self;
    const uint id;
    Source(Timer& self, uint id) : self(self), id(id) {}

    //timer.cpp
    auto step(uint clocks = 1) -> void;
    auto irq() -> void;
    auto reset() -> void;

    uint16 counter;
    uint16 target;
     uint1 sync;
     uint2 mode;
     uint1 resetMode;
     uint1 irqOnTarget;
     uint1 irqOnSaturate;
     uint1 irqRepeat;
     uint1 irqMode;
     uint1 clock;
     uint1 divider;
     uint1 irqLine = 1;  //0 = active
     uint1 reachedTarget;
     uint1 reachedSaturate;
     uint3 unknown;

  //internal:
     uint1 paused;
  } timers[3] = {{*this, 0}, {*this, 1}, {*this, 2}};
};

extern Timer timer;
