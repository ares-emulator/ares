//Zilog Z80

struct APU : Z80, Z80::Bus, Thread {
  Node::Object node;
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
  auto busownerAPU() const -> bool { return (state.resLine & state.busreqLatch) == 0; }  //Z80 has bus access
  auto busownerCPU() const -> bool { return (state.resLine & state.busreqLatch) == 1; }  //68K has bus access

  //z80.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;
  auto power(bool reset) -> void;
  auto restart() -> void;

  auto setNMI(n1 line) -> void;
  auto setINT(n1 line) -> void;
  auto setRES(n1 line) -> void;
  auto setBUSREQ(n1 line) -> void;

  //bus.cpp
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;
  auto readExternal(n24 address) -> n8;
  auto writeExternal(n24 address, n8 data) -> void;

  auto in(n16 address) -> n8 override;
  auto out(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct State {
    n1 nmiLine;
    n1 intLine;
    n1 resLine;
    n1 busreqLine;
    n1 busreqLatch;
    n9 bank;
  } state;
};

extern APU apu;
