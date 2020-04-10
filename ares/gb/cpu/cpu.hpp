struct CPU : SM83, Thread {
  Node::Component node;
  Node::String version;
  Memory::Writable<uint8> wram;  //GB = 8KB, GBC = 32KB
  Memory::Writable<uint8> hram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Memory {
      Node::Memory wram;
      Node::Memory hram;
    } memory;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification interrupt;
    } tracer;
  } debugger;

  struct Interrupt { enum : uint {
    /* 0 */ VerticalBlank,
    /* 1 */ Stat,
    /* 2 */ Timer,
    /* 3 */ Serial,
    /* 4 */ Joypad,
  };};

  auto lowSpeed()  const -> bool { return status.speedDouble == 0; }
  auto highSpeed() const -> bool { return status.speedDouble == 1; }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto raised(uint interrupt) const -> bool;
  auto raise(uint interrupt) -> void;
  auto lower(uint interrupt) -> void;
  auto stoppable() -> bool override;
  auto power() -> void;

  auto serialize(serializer&) -> void;

  //io.cpp
  auto wramAddress(uint13 address) const -> uint16;
  auto input(uint4 data) -> void;
  auto joypPoll() -> void;
  auto readIO(uint cycle, uint16 address, uint8 data) -> uint8;
  auto writeIO(uint cycle, uint16 address, uint8 data) -> void;

  //memory.cpp
  auto stop() -> void override;
  auto halt() -> void override;
  auto idle() -> void override;
  auto read(uint16 address) -> uint8 override;
  auto write(uint16 address, uint8 data) -> void override;
  auto readDMA(uint16 address, uint8 data) -> uint8;
  auto writeDMA(uint13 address, uint8 data) -> void;
  auto readDebugger(uint16 address) -> uint8 override;

  //timing.cpp
  auto step() -> void;
  auto step(uint clocks) -> void;
  auto timer262144hz() -> void;
  auto timer65536hz() -> void;
  auto timer16384hz() -> void;
  auto timer8192hz() -> void;
  auto timer4096hz() -> void;
  auto timer1024hz() -> void;
  auto hblank() -> void;
  auto hblankTrigger() -> void;

  struct Status {
    uint22 clock;
     uint8 interruptLatch;
     uint1 hblankPending;

    //$ff00  JOYP
    uint4 joyp;
    uint1 p14;
    uint1 p15;

    //$ff01  SB
    uint8 serialData;
    uint4 serialBits;

    //$ff02  SC
    uint1 serialClock;
    uint1 serialSpeed;
    uint1 serialTransfer;

    //$ff04  DIV
    uint16 div;

    //$ff05  TIMA
    uint8 tima;

    //$ff06  TMA
    uint8 tma;

    //$ff07  TAC
    uint2 timerClock;
    uint1 timerEnable;

    //$ff0f  IF
    uint5 interruptFlag;

    //$ff4d  KEY1
    uint1 speedSwitch;
    uint1 speedDouble;

    //$ff51,$ff52  HDMA1,HDMA2
    uint16 dmaSource;

    //$ff53,$ff54  HDMA3,HDMA4
    uint16 dmaTarget;

    //$ff55  HDMA5
    uint12 dmaLength;
     uint1 dmaMode;
     uint1 dmaCompleted = 1;

    //$ff6c  ???
    uint1 ff6c;

    //$ff70  SVBK
    uint3 wramBank = 1;

    //$ff72-$ff75  ???
    uint8 ff72;
    uint8 ff73;
    uint8 ff74;
    uint3 ff75;

    //$ffff  IE
    uint8 interruptEnable;
  } status;
};

extern CPU cpu;
