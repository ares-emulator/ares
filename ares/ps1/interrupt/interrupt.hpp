struct Interrupt : Memory::Interface {
  Node::Object node;

  //irq.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto poll() -> void;
  auto level(uint source) -> bool;
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
    auto poll() const { return stat & mask; }
    auto level() const { return stat; }
    auto raise() { line = stat = 1; }
    auto lower() { line = 0; }
    auto acknowledge() { stat = 0; }

    uint1 line;
    uint1 stat;
    uint1 mask;
  };

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
