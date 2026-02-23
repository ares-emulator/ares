struct CPU : Z80, Z80::Bus, Thread {
  Node::Object node;
  Memory::Writable<n8> ram;  //SG-1000 = 1KB, SC-3000 = 2KB

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

  auto synchronizing() const -> bool override { return scheduler.synchronizing(); }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;

  auto setNMI(bool value) -> void;
  auto setIRQ(bool value) -> void;

  auto power() -> void;

  //memory.cpp
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;

  auto in(n16 address) -> n8 override;
  auto out(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct State {
    bool nmiLine = 0;
    bool irqLine = 0;
  } state;
};

extern CPU cpu;
