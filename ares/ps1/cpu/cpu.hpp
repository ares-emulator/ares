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

  struct DMA : Memory::IO<DMA> {
    enum : uint { MDECIN, MDECOUT, GPU, CDROM, SPU, PIO, OTC };  //channels

    CPU& self;
    DMA(CPU& self) : self(self) {}

    //dma.cpp
    auto readWord(u32 address) -> u32;
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
