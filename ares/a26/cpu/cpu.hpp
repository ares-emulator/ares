struct CPU : MOS6502, Thread {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
    } tracer;
  } debugger;

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

  auto rdyLine(bool) -> void;

//protected:
  struct IO {
    n1  rdyLine = 1;
    n32 scanlineCycles = 0;
  } io;
};

extern CPU cpu;
