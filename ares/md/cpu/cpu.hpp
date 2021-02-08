//Motorola 68000

struct CPU : M68K, Thread {
  Node::Object node;

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

  enum class Interrupt : u32 {
    Reset,
    HorizontalBlank,
    VerticalBlank,
  };

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto idle(u32 clocks) -> void override;
  auto wait(u32 clocks) -> void override;

  auto raise(Interrupt) -> void;
  auto lower(Interrupt) -> void;

  auto power(bool reset) -> void;

  //bus.cpp
  auto read(n1 upper, n1 lower, n24 address, n16 data = 0) -> n16 override;
  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override;

  //io.cpp
  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  Memory::Writable<n16> ram;
  Memory::Readable<n16> tmss;
  n1 tmssEnable;

  struct IO {
    b1 version;  //0 = Model 1; 1 = Model 2+
    b1 romEnable;
    b1 vdpEnable[2];
  } io;

  struct Refresh {
    n32 ram;
    n7  external;
  } refresh;

  struct State {
    n32 interruptLine;
    n32 interruptPending;
  } state;
};

extern CPU cpu;
