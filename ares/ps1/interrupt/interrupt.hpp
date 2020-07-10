struct Interrupt {
  Node::Component node;

  //irq.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto poll() -> void;
  auto raise(uint source) -> void;
  auto lower(uint source) -> void;
  auto pulse(uint source) -> void;
  auto drive(uint source, bool line) -> void;
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

  enum : uint { Vblank, GPU, CDROM, DMA, Timer0, Timer1, Timer2, Peripheral, SIO, SPU, PIO };

  struct Source {
    uint1 line = 0;
    uint1 stat = 0;
    uint1 mask = 0;
  };

  uint1  line = 0;
  Source vblank;
  Source gpu;
  Source cdrom;
  Source dma;
  Source timer0;
  Source timer1;
  Source timer2;
  Source peripheral;
  Source sio;
  Source spu;
  Source pio;
};

extern Interrupt interrupt;
