struct CPU : MOS6502, Thread {
  Node::Object node;
  Memory::Writable<n8> ram;  //1KB, mirrored across 0000-1fff

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto setIRQ(bool value) -> void;

  auto power() -> void;

  //memory.cpp
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;
  auto readBus(n16 address) -> n8;
  auto writeBus(n16 address, n8 data) -> void;
  auto readDebugger(n16 address) -> n8 override;

  auto lastCycle() -> void override;
  auto irqPending() -> bool override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //the only interrupt source is a free-running timer: it pulls IRQ low every
  //16384 cycles and releases it ten cycles later. nothing on the board can mask
  //or acknowledge it, so games poll and rely on the CPU's own I flag
  static constexpr u32 TimerPeriod = 32768 / 2;
  static constexpr u32 TimerAssert = 10;

private:
  struct State {
    n1  interruptPending;
    n1  resetPending;
    n1  irqLine;
    u32 timer;
    u32 release;
  } state;
};

extern CPU cpu;
