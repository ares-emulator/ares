//MIPS Interface

struct MI : Memory::IO<MI> {
  Node::Component node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto interrupt(string_view) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Notification interrupt;
      Node::Notification io;
    } tracer;
  } debugger;

  //mi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  enum class IRQ : uint { SP, SI, AI, VI, PI, DP };
  auto raise(IRQ) -> void;
  auto lower(IRQ) -> void;
  auto poll() -> void;

  auto power() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

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

  struct Revision {
    static constexpr u8 io  = 0x02;  //I/O interface
    static constexpr u8 rac = 0x01;  //RAMBUS ASIC cell
    static constexpr u8 rdp = 0x02;  //Reality Display Processor
    static constexpr u8 rsp = 0x02;  //Reality Signal Processor
  } revision;
};

extern MI mi;
