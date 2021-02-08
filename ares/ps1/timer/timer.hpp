struct Timer : Memory::Interface {
  Node::Object node;

  //timer.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(u32 clocks) -> void;
  auto hsync(bool line) -> void;
  auto vsync(bool line) -> void;
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
    u32 dotclock;
    u32 divclock;
  } counter;

  struct Source {
    Timer& self;
    const u32 id;
    Source(Timer& self, u32 id) : self(self), id(id) {}

    //timer.cpp
    auto step(u32 clocks = 1) -> void;
    auto irq() -> void;
    auto reset() -> void;

    n16 counter;
    n16 target;
    n1  synchronize;
    n2  mode;
    n1  resetMode;
    n1  irqOnTarget;
    n1  irqOnSaturate;
    n1  irqRepeat;
    n1  irqMode;
    n1  clock;
    n1  divider;
    n1  irqLine;  //0 = active
    n1  reachedTarget;
    n1  reachedSaturate;
    n3  unknown;

  //internal:
    n1  paused;
    n1  irqTriggered;
  } timers[3] = {{*this, 0}, {*this, 1}, {*this, 2}};
};

extern Timer timer;
