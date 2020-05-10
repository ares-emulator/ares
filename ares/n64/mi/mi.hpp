//MIPS Interface

struct MI {
  enum class IRQ : uint { SP, SI, AI, VI, PI, DP };

  //mi.cpp
  auto raise(IRQ) -> void;
  auto lower(IRQ) -> void;
  auto poll() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Interrupt {
    bool line;
    bool mask;
  };

  struct IRQs {
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
