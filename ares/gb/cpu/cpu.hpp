struct CPU : SM83, Thread {
  Node::Object node;
  Node::Setting::String version;
  Memory::Writable<n8> wram;  //GB = 8KB, GBC = 32KB
  Memory::Writable<n8> hram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Memory {
      Node::Debugger::Memory wram;
      Node::Debugger::Memory hram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger;

  struct Interrupt { enum : u32 {
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
  auto raised(u32 interrupt) const -> bool;
  auto raise(u32 interrupt) -> void;
  auto lower(u32 interrupt) -> void;
  auto stoppable() -> bool override;
  auto power() -> void;

  auto serialize(serializer&) -> void;

  //io.cpp
  auto wramAddress(n13 address) const -> n16;
  auto input(n4 data) -> void;
  auto joypPoll() -> void;
  auto readIO(u32 cycle, n16 address, n8 data) -> n8;
  auto writeIO(u32 cycle, n16 address, n8 data) -> void;

  //memory.cpp
  auto stop() -> void override;
  auto halt() -> void override;
  auto idle() -> void override;
  auto haltBugTrigger() -> void override;
  auto read(n16 address) -> n8 override;
  auto write(n16 address, n8 data) -> void override;
  auto readDMA(n16 address, n8 data) -> n8;
  auto writeDMA(n13 address, n8 data) -> void;
  auto readDebugger(n16 address) -> n8 override;

  //timing.cpp
  auto step() -> void;
  auto step(u32 clocks) -> void;
  auto timer262144hz() -> void;
  auto timer65536hz() -> void;
  auto timer16384hz() -> void;
  auto timer8192hz() -> void;
  auto timer4096hz() -> void;
  auto timer1024hz() -> void;
  auto hblank() -> void;
  auto hblankTrigger() -> void;
  
  struct Status {
    n22 clock;
    n8  interruptLatch;
    n1  hblankPending;

    //$ff00  JOYP
    n4 joyp;
    n1 p14;
    n1 p15;

    //$ff01  SB
    n8 serialData;
    n4 serialBits;

    //$ff02  SC
    n1 serialClock;
    n1 serialSpeed;
    n1 serialTransfer;

    //$ff04  DIV
    n16 div;

    //$ff05  TIMA
    n8 tima;

    //$ff06  TMA
    n8 tma;

    //$ff07  TAC
    n2 timerClock;
    n1 timerEnable;

    //$ff0f  IF
    n5 interruptFlag;

    //$ff4d  KEY1
    n1 speedSwitch;
    n1 speedDouble;

    //$ff51,$ff52  HDMA1,HDMA2
    n16 dmaSource;

    //$ff53,$ff54  HDMA3,HDMA4
    n16 dmaTarget;

    //$ff55  HDMA5
    n7 dmaLength;
    n1 hdmaActive;

    //$ff6c  ???
    n1 ff6c;

    //$ff70  SVBK
    n3 wramBank = 1;

    //$ff72-$ff75  ???
    n8 ff72;
    n8 ff73;
    n8 ff74;
    n3 ff75;

    //$ffff  IE
    n8 interruptEnable;
  } status;
};

extern CPU cpu;
