//Hudson Soft HuC6280

struct CPU : HuC6280, Thread {
  Node::Object node;
  Memory::Writable<n8> ram;  //PC Engine = 8KB, SuperGrafx = 32KB

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
  auto step(u32 clocks) -> void override;
  auto power() -> void;
  auto lastCycle() -> void override;

  //io.cpp
  auto read(n8 bank, n13 address) -> n8 override;
  auto write(n8 bank, n13 address, n8 data) -> void override;
  auto store(n2 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct IRQ2 {  //CD-ROM, BRK instruction
    static constexpr u16 vector = 0xfff6;
    n1 disable;
    n1 pending;
  } irq2;

  struct IRQ1 {  //VDC
    static constexpr u16 vector = 0xfff8;
    n1 disable;
    n1 pending;
  } irq1;

  struct TIQ {  //Timer
    static constexpr u16 vector = 0xfffa;
    n1 disable;
    n1 pending;
  } tiq;

  struct NMI {  //not exposed by the PC Engine
    static constexpr u16 vector = 0xfffc;
  } nmi;

  struct Reset {
    static constexpr u16 vector = 0xfffe;
  } reset;

  struct Timer {
    auto irqLine() const -> bool { return line; }

    n1  line;
    n1  enable;
    n7  reload;
    n7  value;
    i32 counter;
  } timer;

  struct IO {
    n8 buffer;  //latches only on $ff:0800-17ff writes
  } io;
};

extern CPU cpu;
