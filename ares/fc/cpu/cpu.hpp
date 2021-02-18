struct CPU : MOS6502, Thread {
  Node::Object node;
  Memory::Writable<n8> ram;

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

  auto rate() const -> u32 { return Region::PAL() ? 16 : 12; }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto power(bool reset) -> void;

  //memory.cpp
  auto readBus(n16 address) -> n8;
  auto writeBus(n16 address, n8 data) -> void;

  auto readIO(n16 address) -> n8;
  auto writeIO(n16 address, n8 data) -> void;

  auto readDebugger(n16 address) -> n8 override;

  auto serialize(serializer&) -> void;

  //timing.cpp
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;
  auto lastCycle() -> void override;
  auto nmi(n16& vector) -> void override;

  auto oamDMA() -> void;

  auto nmiLine(bool) -> void;
  auto irqLine(bool) -> void;
  auto apuLine(bool) -> void;

  auto rdyLine(bool) -> void;
  auto rdyAddress(bool valid, n16 value = 0) -> void;

//protected:
  struct IO {
    n1  interruptPending;
    n1  nmiPending;
    n1  nmiLine;
    n1  irqLine;
    n1  apuLine;
    n1  rdyLine = 1;
    n1  rdyAddressValid;
    n16 rdyAddressValue;
    n1  oamDMAPending;
    n8  oamDMAPage;
  } io;
};

extern CPU cpu;
