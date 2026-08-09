struct CPU : MOS6502, Thread {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object parent) -> void;
    auto instruction() -> void;
    auto interrupt(string_view type) -> void;

    struct Memory {
      Node::Debugger::Memory bus;
      Node::Debugger::Memory ram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger;

  //cpu.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  //memory.cpp
  auto readBus(n16 address) -> n8;
  auto writeBus(n16 address, n8 data) -> void;
  auto readDebugger(n16 address) -> n8 override;

  //timing.cpp
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;
  auto lastCycle() -> void override;
  auto cancelNmi() -> void override;
  auto delayIrq() -> void override;
  auto irqPending() -> bool override;
  auto nmi(n16& vector) -> void override;

  auto nmiLine(bool line) -> void;
  auto irqLine(bool line) -> void;
  auto rdyLine(bool line) -> void;

private:
  //memory.cpp
  auto peekBus(n16 address) const -> n8;

  struct IO {
    n1 interruptPending;
    n1 resetPending;
    n1 nmiPending;
    n1 nmiLine;
    n1 irqLine;
    n1 rdyLine = 1;
    n8 openBus;
  } io;
};

extern CPU cpu;
