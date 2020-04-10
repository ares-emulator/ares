struct CPU : Z80, Z80::Bus, Thread {
  Node::Component node;
  Memory::Writable<uint8> ram;  //MSX = 64KB, MSX2 = 256KB

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

  auto synchronizing() const -> bool override { return scheduler.synchronizing(); }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void override;

  auto power() -> void;
  auto setIRQ(bool) -> void;

  //memory.cpp
  auto read(uint16 address) -> uint8 override;
  auto write(uint16 address, uint8 data) -> void override;

  auto in(uint16 address) -> uint8 override;
  auto out(uint16 address, uint8 data) -> void override;

  auto readPrimarySlot() -> uint8;
  auto writePrimarySlot(uint8 data) -> void;

  auto readSecondarySlot() -> uint8;
  auto writeSecondarySlot(uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Slot {
    uint8 memory;
    uint2 primary;
    uint2 secondary[4];
  } slot[4];

  struct IO {
    uint1 irqLine;
  } io;
};

extern CPU cpu;
