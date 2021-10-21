//Motorola 68000

struct CPU : M68000, Thread {
  Node::Object node;
  Memory::Readable<n16> tmss;
  Memory::Writable<n16> ram;

  // Bus locking is unavailable for the main cpu of MegaDrive models 1 & 2,
  // preventing the TAS instruction from working correctly in most cases.
  auto lockable() -> bool override { return false; }

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
    External,
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
  auto lower(Interrupt) -> bool;

  auto power(bool reset) -> void;

  //bus.cpp
  auto read(n1 upper, n1 lower, n24 address, n16 _ = 0) -> n16 override;
  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override;

  //io.cpp
  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n1 tmssEnable;

  struct IO {
    b1 version;  //0 = Model 1; 1 = Model 2+
    b1 romEnable;
    b1 vdpEnable[2];
  } io;

  struct Refresh {
    n8 ram;
    n7 external;
  } refresh;

  struct State {
    n32 interruptPending;
  } state;

  int cyclesUntilSync = 0;
  int minCyclesBetweenSyncs = 0;
};

extern CPU cpu;
