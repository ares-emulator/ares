//Zilog Z80

struct APU : Z80, Z80::Bus, Thread {
  Node::Object node;
  Memory::Readable<n8> ram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
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

  auto synchronizing() const -> bool override { return false; }

  //z80.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;
  auto power(bool reset) -> void;

  //memory.cpp
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;

  auto in(n16 address) -> n8 override;
  auto out(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Communication {
    n8 input;
    n8 output;
  } communication;

  struct NMI {
    n1 pending;
    n1 enable;
  } nmi;

  struct IRQ {
    n1 pending;
  } irq;

  struct ROM {
    n8 bankA = 0x02;
    n8 bankB = 0x06;
    n8 bankC = 0x0e;
    n8 bankD = 0x1e;
  } rom;
};

extern APU apu;
