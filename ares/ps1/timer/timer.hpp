struct Timer {
  Node::Component node;

  //timer.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(uint clocks) -> void;
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
     uint1 irqLine;
     uint1 reachedTarget;
     uint1 reachedSaturate;
     uint3 unknown;
  };

  Source htimer;  //Hblank timer 0
  Source vtimer;  //Vblank timer 1
  Source ftimer;  //free timer 2
};

extern Timer timer;
