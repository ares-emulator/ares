struct CPU : WDC65816, Thread, PPUcounter {
  Node::Object node;
  Node::Setting::Natural version;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;
    auto dma(n8 channelID, n24 addressA, n8 addressB, n8 data) -> void;

    struct Memory {
      Node::Debugger::Memory wram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
      Node::Debugger::Tracer::Notification dma;
    } tracer;
  } debugger;

  auto interruptPending() const -> bool override { return status.interruptPending; }
  auto pio() const -> n8 { return io.pio; }
  auto refresh() const -> bool { return status.dramRefresh == 1; }
  auto synchronizing() const -> bool override { return scheduler.synchronizing(); }

  //cpu.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto map() -> void;
  auto power(bool reset) -> void;

  //dma.cpp
  auto dmaEnable() -> bool;
  auto hdmaEnable() -> bool;
  auto hdmaActive() -> bool;

  auto dmaRun() -> void;
  auto hdmaReset() -> void;
  auto hdmaSetup() -> void;
  auto hdmaRun() -> void;

  //memory.cpp
  auto idle() -> void override;
  auto read(n24 address) -> n8 override;
  auto write(n24 address, n8 data) -> void override;
  auto wait(n24 address) const -> u32;
  auto readDisassembler(n24 address) -> n8 override;

  //io.cpp
  auto readRAM(n24 address, n8 data) -> n8;
  auto readAPU(n24 address, n8 data) -> n8;
  auto readCPU(n24 address, n8 data) -> n8;
  auto readDMA(n24 address, n8 data) -> n8;
  auto writeRAM(n24 address, n8 data) -> void;
  auto writeAPU(n24 address, n8 data) -> void;
  auto writeCPU(n24 address, n8 data) -> void;
  auto writeDMA(n24 address, n8 data) -> void;

  //timing.cpp
  auto dmaCounter() const -> u32;
  auto joypadCounter() const -> u32;

  auto step(u32 clocks) -> void;
  auto scanline() -> void;

  auto aluEdge() -> void;
  auto dmaEdge() -> void;
  auto joypadEdge() -> void;

  //irq.cpp
  auto nmiPoll() -> void;
  auto irqPoll() -> void;
  auto nmitimenUpdate(n8 data) -> void;
  auto rdnmi() -> bool;
  auto timeup() -> bool;

  auto nmiTest() -> bool;
  auto irqTest() -> bool;
  auto lastCycle() -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8 wram[128_KiB];
  vector<Thread*> coprocessors;
  vector<Thread*> peripherals;

private:
  struct Counter {
    u32 cpu = 0;
    u32 dma = 0;
  } counter;

  struct Status {
    u32  clockCount = 0;

    bool irqLock = 0;

    u32  dramRefreshPosition = 0;
    u32  dramRefresh = 0;  //0 = not refreshed; 1 = refresh active; 2 = refresh inactive

    u32  hdmaSetupPosition = 0;
    bool hdmaSetupTriggered = 0;

    u32  hdmaPosition = 0;
    bool hdmaTriggered = 0;

    boolean nmiValid;
    boolean nmiLine;
    boolean nmiTransition;
    boolean nmiPending;
    boolean nmiHold;

    boolean irqValid;
    boolean irqLine;
    boolean irqTransition;
    boolean irqPending;
    boolean irqHold;

    bool resetPending = 0;
    bool interruptPending = 0;

    bool dmaActive = 0;
    bool dmaPending = 0;
    bool hdmaPending = 0;
    bool hdmaMode = 0;  //0 = init, 1 = run

    u32  autoJoypadCounter = 33;  //state machine; 4224 / 128 = 33 (inactive)
  } status;

  struct IO {
    //$2181-$2183
    n17 wramAddress;

    //$4200
    boolean hirqEnable;
    boolean virqEnable;
    boolean irqEnable;
    boolean nmiEnable;
    boolean autoJoypadPoll;

    //$4201
    n8 pio = 0xff;

    //$4202-$4203
    n8 wrmpya = 0xff;
    n8 wrmpyb = 0xff;

    //$4204-$4206
    n16 wrdiva = 0xffff;
    n8  wrdivb = 0xff;

    //$4207-$420a
    n12 htime = 0x1ff + 1 << 2;
    n9  vtime = 0x1ff;

    //$420d
    u32 romSpeed = 8;

    //$4210
    n4 version = 2;  //allowed values: 1, 2

    //$4214-$4217
    n16 rddiv;
    n16 rdmpy;

    //$4218-$421f
    n16 joy1;
    n16 joy2;
    n16 joy3;
    n16 joy4;
  } io;

  struct ALU {
    u32 mpyctr = 0;
    u32 divctr = 0;
    u32 shift = 0;
  } alu;

  struct Channel {
    //dma.cpp
    auto step(u32 clocks) -> void;
    auto edge() -> void;

    auto validA(n24 address) -> bool;
    auto readA(n24 address) -> n8;
    auto readB(n8 address, bool valid) -> n8;
    auto writeA(n24 address, n8 data) -> void;
    auto writeB(n8 address, n8 data, bool valid) -> void;
    auto transfer(n24 address, n2 index) -> void;

    auto dmaRun() -> void;
    auto hdmaActive() -> bool;
    auto hdmaFinished() -> bool;
    auto hdmaReset() -> void;
    auto hdmaSetup() -> void;
    auto hdmaReload() -> void;
    auto hdmaTransfer() -> void;
    auto hdmaAdvance() -> void;

    //$420b
    n1 dmaEnable;

    //$420c
    n1 hdmaEnable;

    //$43x0
    n3 transferMode = 7;
    n1 fixedTransfer = 1;
    n1 reverseTransfer = 1;
    n1 unused = 1;
    n1 indirect = 1;
    n1 direction = 1;

    //$43x1
    n8 targetAddress = 0xff;

    //$43x2-$43x3
    n16 sourceAddress = 0xffff;

    //$43x4
    n8 sourceBank = 0xff;

    //$43x5-$43x6
    union {
      n16 transferSize;
      n16 indirectAddress;
    };

    //$43x7
    n8 indirectBank = 0xff;

    //$43x8-$43x9
    n16 hdmaAddress = 0xffff;

    //$43xa
    n8 lineCounter = 0xff;

    //$43xb/$43xf
    n8 unknown = 0xff;

    //internal state
    n1 hdmaCompleted;
    n1 hdmaDoTransfer;

    //unserialized:
    n8 id;
    maybe<Channel&> next;

    Channel() : transferSize(0xffff) {}
  } channels[8];
};

extern CPU cpu;
