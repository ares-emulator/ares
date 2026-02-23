struct Interrupt : Memory::Interface {
  Node::Object node;

  //interrupt.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto poll() -> void;
  auto level(u32 source) -> bool;
  auto raise(u32 source) -> void;
  auto lower(u32 source) -> void;
  auto pulse(u32 source) -> void;
  auto drive(u32 source, bool line) -> void;
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

  enum : u32 { Vblank, GPU, CDROM, DMA, Timer0, Timer1, Timer2, Peripheral, SIO, SPU, PIO };

  struct Source {
    auto poll() const { return stat & mask; }
    auto level() const { return stat; }
    auto raise() { line = stat = 1; }
    auto lower() { line = 0; }
    auto acknowledge() { stat = 0; }

    n1 line;
    n1 stat;
    n1 mask;
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
