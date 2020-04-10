//Hudson Soft HuC6280

struct CPU : HuC6280, Thread {
  Node::Component node;
  Memory::Writable<uint8> ram;  //PC Engine = 8KB, SuperGrafx = 32KB

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Memory {
      Node::Memory ram;
    } memory;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification interrupt;
    } tracer;
  } debugger;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void override;
  auto power() -> void;
  auto lastCycle() -> void override;

  //io.cpp
  auto read(uint8 bank, uint13 address) -> uint8 override;
  auto write(uint8 bank, uint13 address, uint8 data) -> void override;
  auto store(uint2 address, uint8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct IRQ2 {  //CD-ROM, BRK instruction
    static constexpr uint16_t vector = 0xfff6;
    uint1 disable;
    uint1 pending;
  } irq2;

  struct IRQ1 {  //VDC
    static constexpr uint16_t vector = 0xfff8;
    uint1 disable;
    uint1 pending;
  } irq1;

  struct TIQ {  //Timer
    static constexpr uint16_t vector = 0xfffa;
    uint1 disable;
    uint1 pending;
  } tiq;

  struct NMI {  //not exposed by the PC Engine
    static constexpr uint16_t vector = 0xfffc;
  } nmi;

  struct Reset {
    static constexpr uint16_t vector = 0xfffe;
  } reset;

  struct Timer {
    auto irqLine() const -> bool { return line; }

    uint1 line;
    uint1 enable;
    uint7 reload;
    uint7 value;
    int32 counter;
  } timer;

  struct IO {
    uint8 buffer;  //latches only on $ff:0800-17ff writes
  } io;
};

extern CPU cpu;
