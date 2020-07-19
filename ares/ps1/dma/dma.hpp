struct DMA {
  Node::Component node;

  //dma.cpp
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

  //transfer.cpp
  auto transferLinear(uint c) -> void;
  auto transferLinked(uint c) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  enum : uint { MDECIN, MDECOUT, GPU, CDROM, SPU, PIO, OTC };  //channels

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
    //dma.cpp
    auto active() const -> bool {
      if(synchronization == 0) return enable && trigger;
      return enable;
    }

     uint1 masterEnable;
     uint3 priority;
    uint24 address;
    uint16 length;
    uint16 blocks;
     uint1 direction;
     uint1 step;  //0 = increment; 1 = decrement
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
  } channel[7];
};

extern DMA dma;
