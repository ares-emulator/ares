struct CPU : MOS6502, Thread {
  Node::Component node;
  Memory::Writable<uint8> ram;

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

  auto rate() const -> uint { return Region::PAL() ? 16 : 12; }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power(bool reset) -> void;

  //memory.cpp
  auto readBus(uint16 address) -> uint8;
  auto writeBus(uint16 address, uint8 data) -> void;

  auto readIO(uint16 address) -> uint8;
  auto writeIO(uint16 address, uint8 data) -> void;

  auto readDebugger(uint16 address) -> uint8 override;

  auto serialize(serializer&) -> void;

  //timing.cpp
  auto read(uint16 address) -> uint8 override;
  auto write(uint16 address, uint8 data) -> void override;
  auto lastCycle() -> void override;
  auto nmi(uint16& vector) -> void override;

  auto oamDMA() -> void;

  auto nmiLine(bool) -> void;
  auto irqLine(bool) -> void;
  auto apuLine(bool) -> void;

  auto rdyLine(bool) -> void;
  auto rdyAddress(bool valid, uint16 value = 0) -> void;

//protected:
  struct IO {
     uint1 interruptPending;
     uint1 nmiPending;
     uint1 nmiLine;
     uint1 irqLine;
     uint1 apuLine;
     uint1 rdyLine = 1;
     uint1 rdyAddressValid;
    uint16 rdyAddressValue;
     uint1 oamDMAPending;
     uint8 oamDMAPage;
  } io;
};

extern CPU cpu;
