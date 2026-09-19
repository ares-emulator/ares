struct CPU : I8080, I8080::Bus, Thread {
  Node::Object node;

  //2KB of RAM is the only writable memory in the machine; the video hardware
  //scans it directly, so this doubles as VRAM. see vdp.cpp
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

  auto synchronizing() const -> bool override { return scheduler.synchronizing(); }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;

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
    bool irqLine = 0;
  } state;
};

extern CPU cpu;
