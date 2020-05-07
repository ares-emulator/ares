//MIPS Interface

struct MI {
  //mi.cpp
  auto pollInterrupts() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Interrupt {
    bool line;
    bool mask;
  };

  struct IRQ {
    Interrupt sp;
    Interrupt si;
    Interrupt ai;
    Interrupt vi;
    Interrupt pi;
    Interrupt dp;
  } irq;

  struct IO {
  } io;
};

extern MI mi;
