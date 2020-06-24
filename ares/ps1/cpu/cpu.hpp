//LSI CoreWare CW33300 (MIPS R3000A core)

struct CPU : Thread {
  Node::Component node;
  Memory::Writable ram;
  Memory::Writable cache;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  #include "core/core.hpp"

  struct Interrupt {
    enum : uint { Vblank, GPU, CDROM, DMA, Timer0, Timer1, Timer2, Peripheral, SIO, SPU, PIO };

    CPU& self;
    Interrupt(CPU& self) : self(self) {}

    //interrupt.cpp
    auto poll() -> void;
    auto pulse(uint source) -> void;
    auto raise(uint source) -> void;
    auto lower(uint source) -> void;

    auto readByte(u32 address) -> u8;
    auto readHalf(u32 address) -> u16;
    auto readWord(u32 address) -> u32;
    auto writeByte(u32 address, u8 data) -> void;
    auto writeHalf(u32 address, u16 data) -> void;
    auto writeWord(u32 address, u32 data) -> void;

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
  } interrupt{*this};

  struct DMA {
    enum : uint { MDECIN, MDECOUT, GPU, CDROM, SPU, PIO, OTC };  //channels

    CPU& self;
    DMA(CPU& self) : self(self) {}

    //dma.cpp
    auto readByte(u32 address) -> u8;
    auto readHalf(u32 address) -> u16;
    auto readWord(u32 address) -> u32;
    auto writeByte(u32 address, u8 data) -> void;
    auto writeHalf(u32 address, u16 data) -> void;
    auto writeWord(u32 address, u32 data) -> void;
    auto transferLinear(uint c) -> void;
    auto transferLinked(uint c) -> void;
    auto pollIRQ() -> void;
    auto power(bool reset) -> void;

    struct IRQ {
      uint1 force;
      uint1 enable;
      uint1 flag;
      uint6 unknown;
    } irq;

    struct Channel {
      //dma.cpp
      auto active() const -> bool;

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
  } dma{*this};
};

extern CPU cpu;
