struct Timer {
  Node::Component node;

  //timer.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(uint clocks) -> void;
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

  struct Source {
    Timer& self;
    Source(Timer& self) : self(self) {}

    //timer.cpp
    auto step(uint clocks) -> void;
    auto irq() -> void;

    uint16 counter;
    uint16 target;
     uint1 synchronizeEnable;
     uint2 synchronizeMode;
     uint1 resetMode;
     uint1 irqOnTarget;
     uint1 irqOnSaturate;
     uint1 irqRepeat;
     uint1 irqMode;
     uint2 clockSource;
     uint1 irqLine = 1;  //0 = active
     uint1 reachedTarget;
     uint1 reachedSaturate;
     uint3 unknown;
  };

  Source htimer{*this};  //Hblank timer 0
  Source vtimer{*this};  //Vblank timer 1
  Source ftimer{*this};  //free timer 2
};

extern Timer timer;
