struct CPU : V30MZ, Thread, IO {
  Node::Object node;

  struct Debugger {
    CPU& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto ports() -> string;
    auto instruction() -> void;
    auto interrupt(n3) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger{*this};

  struct Interrupt { enum : u32 {
    SerialSend,
    Input,
    Cartridge,
    SerialReceive,
    LineCompare,
    VblankTimer,
    Vblank,
    HblankTimer,
  };};

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;
  auto width(n20 address) -> u32 override;
  auto speed(n20 address) -> n32 override;
  auto read(n20 address) -> n8 override;
  auto write(n20 address, n8 data) -> void override;
  auto in(n16 port) -> n8 override;
  auto out(n16 port, n8 data) -> void override;
  auto ioWidth(n16 port) -> u32 override;
  auto ioSpeed(n16 port) -> n32 override;

  auto power() -> void;

  //io.cpp
  auto readIO(n16 address) -> n8 override;
  auto writeIO(n16 address, n8 data) -> void override;

  //interrupt.cpp
  auto poll() -> void;
  auto irqLevel(n3, bool) -> void;
  auto raise(n3) -> void;
  auto lower(n3) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct DMA {
    CPU& self;

    //dma.cpp
    auto valid(n20 address) -> bool;
    auto transfer() -> void;

    n20 source;
    n16 target;
    n16 length;
    n1  enable;
    n1  direction;  //0 = increment, 1 = increment
  } dma{*this};

  struct Keypad {
    CPU& self;

    //keypad.cpp
    auto read() -> n4;
    auto poll() -> void;
    auto power() -> void;

    n3 matrix;
    n3 lastPolledMatrix;
  } keypad{*this};

  struct IO {
    n1 cartridgeEnable;
    n1 cartridgeRomWidth; // 0 = 8-bit; 1 = 16-bit
    n1 cartridgeRomWait; // 0 = +0 cycles; 1 = +1 cycle
    n1 cartridgeClock;
    n1 cartridgeSramWait; // 0 = +0 cycles; 1 = +1 cycle
    n1 cartridgeIoWait; // 0 = +0 cycles; 1 = +1 cycle
    n8 interruptBase;
    n8 interruptEnable;
    n8 interruptStatus;
    n8 interruptLevel;
    n1 nmiOnLowBattery;
  } io;
};

extern CPU cpu;
