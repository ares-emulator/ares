//Hudson Soft HuC6280

struct CPU : HuC6280, Thread {
  Node::Component node;
  Node::Instruction eventInstruction;
  Node::Notification eventInterrupt;
  Memory::Writable<uint8> ram;  //PC Engine = 8KB, SuperGrafx = 32KB

  //cpu.cpp
  auto load(Node::Object, Node::Object) -> void;
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
    enum : uint16_t { vector = 0xfff6 };
    uint1 disable;
    uint1 pending;
  } irq2;

  struct IRQ1 {  //VDC
    enum : uint16_t { vector = 0xfff8 };
    uint1 disable;
    uint1 pending;
  } irq1;

  struct TIQ {  //Timer
    enum : uint16_t { vector = 0xfffa };
    uint1 disable;
    uint1 pending;
  } tiq;

  struct NMI {  //not exposed by the PC Engine
    enum : uint16_t { vector = 0xfffc };
  } nmi;

  struct Reset {
    enum : uint16_t { vector = 0xfffe };
  } reset;

  struct Timer {
    inline auto irqLine() const { return line; }

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
