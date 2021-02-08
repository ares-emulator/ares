struct DMA : Thread, Memory::Interface {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto transfer(u32 channel) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification dma;
    } tracer;
  } debugger;

  //dma.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
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

  enum : u32 { MDECin, MDECout, GPU, CDROM, SPU, PIO, OTC };  //channel IDs
  enum : u32 { Waiting, Running };  //channel states

  struct IRQ {
    DMA& self;
    IRQ(DMA& self) : self(self) {}

    //irq.cpp
    auto poll() -> void;

    n1 force;
    n1 enable;
    n1 flag;
    n6 unknown;
  } irq{*this};

  struct Channel {
    const u32 id;

    //channel.cpp
    auto step(u32 clocks) -> bool;
    auto transferBlock() -> void;
    auto transferChain() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1  masterEnable;
    n3  priority;
    n24 address;
    n16 length;
    n16 blocks;
    n1  direction;
    n1  decrement;
    n2  synchronization;
    struct Chopping {
      n1 enable;
      n3 dmaWindow;
      n3 cpuWindow;
    } chopping;
    n1 enable;
    n1 trigger;
    n2 unknown;
    struct IRQ {
      n1 enable;
      n1 flag;
    } irq;
    struct Chain {
      n24 address;
      n8  length;
    } chain;

    n8  state;
    i32 counter;
  } channels[7] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}};

  u32 channelsByPriority[7];
};

extern DMA dma;
