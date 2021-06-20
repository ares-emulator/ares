struct CPU : V30MZ, Thread, IO {
  Node::Object node;

  struct Debugger {
    CPU& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
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

  auto read(n20 address) -> n8 override;
  auto write(n20 address, n8 data) -> void override;
  auto in(n16 port) -> n8 override;
  auto out(n16 port, n8 data) -> void override;

  auto power() -> void;

  //io.cpp
  auto readIO(n16 address) -> n8 override;
  auto writeIO(n16 address, n8 data) -> void override;

  //interrupt.cpp
  auto poll() -> void;
  auto raise(n3) -> void;
  auto lower(n3) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct DMA {
    CPU& self;

    //dma.cpp
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

    n3 matrix;
  } keypad{*this};

  struct IO {
    n1 cartridgeEnable;
    n8 interruptBase;
    n8 interruptEnable;
    n8 interruptStatus;
    n8 serialData;
    n1 serialBaudRate;  //0 = 9600; 1 = 38400
    n1 serialEnable;
  } io;
};

extern CPU cpu;
