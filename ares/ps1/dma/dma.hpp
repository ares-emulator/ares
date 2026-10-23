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
  auto arbitrate() -> bool;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  auto active() -> bool;

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
  enum : u32 { Idle, Running };  //channel states

  struct IRQ {
    DMA& self;
    IRQ(DMA& self) : self(self) {}

    //irq.cpp
    auto poll() -> void;

    n1 force;
    n1 enable;
    n1 flag;
    n7 unknown;
  } irq{*this};

  struct Channel {
    const u32 id;

    //channel.cpp
    auto step() -> bool;
    auto ready() -> bool;
    auto transferClocks() -> u32;
    auto transferBlock() -> void;
    auto transferChain() -> void;
    auto kick() -> bool;
    auto acceptRequest() -> bool;
    auto signalIRQ(bool segment = false) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1  masterEnable;
    n3  priority;
    n24 baseAddress;  //CPU-visible MADR, published at mode-specific boundaries
    n16 baseLength;   //CPU-visible BCR count/size
    n24 address;      //internal word cursor
    n16 length;       //internal burst count or request block size
    n16 blocks;
    n1  direction;
    n1  decrement;
    n2  synchronization;
    struct Chopping {
      n1 enable;
      n3 dmaWindow;
      n3 cpuWindow;
      u32 remaining;
    } chopping;
    n1 enable;
    n1 trigger;
    n1 forced;  //accepted block/node originated from software force, not DREQ
    n2 unknown;
    struct IRQ {
      n1 enable;
      n1 flag;
    } irq;
    struct Chain {
      n24 address;
      n8  length;
      u32 transferred;
    } chain;

    n8  state;
    u32 blockOffset;
  } channels[7] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}};

  u32 channelsByPriority[7];
  i32 counter;
  n4 cpuControl;  //DPCR bits 28-31; scheduling ownership is separate
};

extern DMA dma;
