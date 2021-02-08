struct CPU : V30MZ, Thread, IO {
  Node::Object node;

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

  enum class Interrupt : u32 {
    SerialSend,
    Input,
    Cartridge,
    SerialReceive,
    LineCompare,
    VblankTimer,
    Vblank,
    HblankTimer,
  };

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto wait(u32 clocks = 1) -> void override;
  auto read(n20 addr) -> n8 override;
  auto write(n20 addr, n8 data) -> void override;
  auto in(n16 port) -> n8 override;
  auto out(n16 port, n8 data) -> void override;

  auto power() -> void;

  //io.cpp
  auto keypadRead() -> n4;
  auto portRead(n16 address) -> n8 override;
  auto portWrite(n16 address, n8 data) -> void override;

  //interrupt.cpp
  auto poll() -> void;
  auto raise(Interrupt) -> void;
  auto lower(Interrupt) -> void;

  //dma.cpp
  auto dmaTransfer() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Registers {
    //$0040-0042  DMA_SRC
    n20 dmaSource;

    //$0044-0045  DMA_DST
    n16 dmaTarget;

    //$0046-0047  DMA_LEN
    n16 dmaLength;

    //$0048  DMA_CTRL
    n1 dmaEnable;
    n1 dmaMode;  //0 = increment; 1 = decrement

    //$00a0  HW_FLAGS
    n1 cartridgeEnable;

    //$00b0  INT_BASE
    n8 interruptBase;

    //$00b1  SER_DATA
    n8 serialData;

    //$00b2  INT_ENABLE
    n8 interruptEnable;

    //$00b3  SER_STATUS
    n1 serialBaudRate;  //0 = 9600; 1 = 38400
    n1 serialEnable;

    //$00b4  INT_STATUS
    n8 interruptStatus;

    //$00b5  KEYPAD
    n3 keypadMatrix;
  } r;
};

extern CPU cpu;
