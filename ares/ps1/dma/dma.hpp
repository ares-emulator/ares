struct DMA : Thread, Memory::Interface {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto transfer(uint channel) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification dma;
    } tracer;
  } debugger;

  //dma.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //channel.cpp
  auto sortChannelsByPriority() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  enum : uint { MDECin, MDECout, GPU, CDROM, SPU, PIO, OTC };  //channel IDs
  enum : uint { Waiting, Running };  //channel states

  struct IRQ {
    DMA& self;
    IRQ(DMA& self) : self(self) {}

    //irq.cpp
    auto poll() -> void;

    uint1 force;
    uint1 enable;
    uint1 flag;
    uint6 unknown;
  } irq{*this};

  struct Channel {
    const uint id;

    //channel.cpp
    auto step(uint clocks) -> bool;
    auto transferBlock() -> void;
    auto transferChain() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

     uint1 masterEnable;
     uint3 priority;
    uint24 address;
    uint16 length;
    uint16 blocks;
     uint1 direction;
     uint1 decrement;
     uint2 synchronization;
    struct Chopping {
      uint1 enable;
      uint3 dmaWindow;
      uint3 cpuWindow;
    } chopping;
    uint1 enable;
    uint1 trigger;
    uint2 unknown;
    struct IRQ {
      uint1 enable;
      uint1 flag;
    } irq;
    struct Chain {
      uint24 address;
       uint8 length;
    } chain;

    uint8 state;
    int32 counter;
  } channels[7] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}};

  uint channelsByPriority[7];
};

extern DMA dma;
