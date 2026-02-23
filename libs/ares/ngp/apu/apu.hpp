struct APU : Z80, Z80::Bus, Thread {
  Node::Object node;
  Memory::Writable<n8> ram;

  struct Debugger {
    APU& self;

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
  } debugger{*this};

  auto synchronizing() const -> bool override { return scheduler.synchronizing(); }

  //apu.cpp
  auto load(Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;
  auto power() -> void;
  auto enable() -> void;
  auto disable() -> void;

  //memory.cpp
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;

  auto in(n16 address) -> n8 override;
  auto out(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct NMI {
    n1 line;
  } nmi;

  struct IRQ {
    n1 line;
  } irq;

  struct Port {
    n8 data;
  } port;

  struct IO {
    n1 enable;
  } io;
};

extern APU apu;
