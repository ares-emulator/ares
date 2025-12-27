struct CPU : ARM7TDMI, Thread, IO {
  Node::Object node;
  Memory::Writable<n8> iwram;  // 32KB
  Memory::Writable<n8> ewram;  //256KB

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view type) -> void;

    struct Memory {
      Node::Debugger::Memory iwram;
      Node::Debugger::Memory ewram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;  //todo: ARM7TDMI needs to notify CPU when interrupts occur
    } tracer;
  } debugger;

  struct Interrupt { enum : u32 {
    VBlank       = 0x0001,
    HBlank       = 0x0002,
    VCoincidence = 0x0004,
    Timer0       = 0x0008,
    Timer1       = 0x0010,
    Timer2       = 0x0020,
    Timer3       = 0x0040,
    Serial       = 0x0080,
    DMA0         = 0x0100,
    DMA1         = 0x0200,
    DMA2         = 0x0400,
    DMA3         = 0x0800,
    Keypad       = 0x1000,
    Cartridge    = 0x2000,
  };};

  auto clock() const -> u32 { return context.clock; }
  auto halted() const -> bool { return context.halted; }
  auto stopped() const -> bool { return context.stopped; }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto setInterruptFlag(u32 source) -> void;
  auto stepIRQ() -> void;
  auto step(u32 clocks) -> void override;
  auto power() -> void;

  //prefetch.cpp
  auto prefetchSync(u32 mode, n32 address) -> void;
  auto prefetchStepInternal(u32 clocks) -> void;
  auto prefetchStep(u32 clocks) -> void;
  auto prefetchReset() -> void;
  auto prefetchRead() -> n16;

  //bus.cpp
  auto sleep() -> void override;
  template<bool isDMA, bool UseDebugger> auto getBus(u32 mode, n32 address) -> n32;
  auto get(u32 mode, n32 address) -> n32 override;
  auto getDMA(u32 mode, n32 address) -> n32;
  auto getDebugger(u32 mode, n32 address) -> n32 override;
  template<bool IsDMA> auto setBus(u32 mode, n32 address, n32 word) -> void;
  auto set(u32 mode, n32 address, n32 word) -> void override;
  auto setDMA(u32 mode, n32 address, n32 word) -> void;
  auto lock() -> void override;
  auto unlock() -> void override;
  auto waitCartridge(n32 address, bool sequential) -> u32;
  template<bool IsDMA> auto checkBurst(u32 mode) -> bool;

  //io.cpp
  auto readIO(n32 address) -> n8 override;
  auto writeIO(n32 address, n8 byte) -> void override;

  template<bool UseDebugger> auto readIWRAM(u32 mode, n32 address) -> n32;
  auto writeIWRAM(u32 mode, n32 address, n32 word) -> void;

  template<bool UseDebugger> auto readEWRAM(u32 mode, n32 address) -> n32;
  auto writeEWRAM(u32 mode, n32 address, n32 word) -> void;

  template<bool UseDebugger> auto readPRAM(u32 mode, n32 address) -> n32;
  auto writePRAM(u32 mode, n32 address, n32 word) -> void;

  template<bool UseDebugger> auto readVRAM(u32 mode, n32 address) -> n32;
  auto writeVRAM(u32 mode, n32 address, n32 word) -> void;

  template<bool UseDebugger> auto readROM(u32 mode, n32 address) -> n32;
  auto writeROM(u32 mode, n32 address, n32 word) -> void;

  //dma.cpp
  auto dmaVblank() -> void;
  auto dmaHblank() -> void;
  auto dmaHDMA() -> void;

  //timer.cpp
  auto runFIFO(u32 n) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct uintVN {
    auto operator()() const -> n32 { return data & mask; }
    auto setBits(u32 bits) -> void { mask = (1 << bits) - 1; }

    n32 data;
    n32 mask;
  };

  struct DMAC {
    //dma.cpp
    auto step() -> bool;
    auto runPending() -> void;

    n1  romBurst;
    n1  active;
    n2  activeChannel;
    n1  stallingCPU;
    n1  writeCycle;

    struct Channel {
      auto ready() -> bool;
      auto read() -> void;
      auto write() -> void;

      n2 id;

      n1 active;
      i32 waiting;

      n2 targetMode;
      n2 sourceMode;
      n1 repeat;
      n1 size;
      n1 drq;
      n2 timingMode;
      n1 irq;
      n1 enable;

      uintVN source;
      uintVN target;
      uintVN length;

      struct Latch {
        uintVN source;
        uintVN target;
        uintVN length;
        u32 data;
      } latch;
    } channel[4];
  } dmac;

  struct Timer {
    //timer.cpp
    auto stepLatch() -> void;
    auto reloadLatch() -> void;
    auto run() -> void;
    auto step() -> void;

    n2  id;

    n1  pending;

    n16 period;
    n16 reload;

    n2  frequency;
    n1  cascade;
    n1  irq;
    n1  enable;
    
    struct Latch {
      n16 reload;
      n8  control;
      n2  reloadFlags;
      n1  controlFlag;
    } latch;
  } timer[4];

  struct Serial {
    n1  shiftClockSelect;
    n1  shiftClockFrequency;
    n1  transferEnableReceive;
    n1  transferEnableSend;
    n1  startBit;
    n4  uartFlags;
    n2  mode;
    n1  irqEnable;

    n16 data[4];
    n16 dataMulti;
  } serial;

  struct Keypad {
    //keypad.cpp
    auto run() -> void;

    n1 enable;
    n1 condition;
    n1 flag[10];
    n1 keyStates[10];

    n1 conditionMet;
  } keypad;

  struct Joybus {
    n1  sc;
    n1  sd;
    n1  si;
    n1  so;
    n1  scMode;
    n1  sdMode;
    n1  siMode;
    n1  soMode;
    n1  siIRQEnable;
    n2  mode;

    n1  resetSignal;
    n1  receiveComplete;
    n1  sendComplete;
    n1  resetIRQEnable;

    n32 receive;
    n32 transmit;

    n1  receiveFlag;
    n1  sendFlag;
    n2  generalFlag;
  } joybus;

  struct IRQ {
    n1  ime[2];
    n1  synchronizer;
    n16 enable[2];
    n16 flag[2];
  } irq;

  struct Wait {
    n2 nwait[4];
    n1 swait[4];
    n2 phi;
    n1 prefetch;
    n1 gameType;
  } wait;

  struct Memory {
    n1 biosSwap;
    n2 unknown1;
    n1 cgbBootRomDisable;
    n1 ewram = 1;
    n4 ewramWait = 13;
    n4 unknown2;
  } memory;

  struct OpenBus {
    auto get(u32 mode, n32 address) -> n32;
    auto set(u32 mode, n32 address, n32 word) -> void;
    n32 data;
    n32 iwramData;
  } openBus;

  struct {
    auto empty() const { return addr == load; }
    auto full() const { return load - addr == 16; }
    auto size() const { return (load - addr) >> 1; }

    n16 slot[8];
    n32 addr;      //read location of slot buffer
    n32 load;      //write location of slot buffer
    i32 wait = 1;  //number of clocks before next slot load
    n1  stopped = 1;
    n1  ahead;
  } prefetch;

  struct Coprocessor {
    //coprocessor.cpp
    auto vcCDP() -> void;
    auto debugMCR() -> void;
    auto debugMRC() -> n32;
  } coprocessor;

  struct Context {
    n32 clock;
    n1  halted;
    n1  stopped;
    n1  booted;  //set to true by the GBA BIOS
    n1  romAccess;
    n1  timerLatched;
    n1  busLocked;
    n1  burstActive;
    n32 hcounter;
  } context;
};

extern CPU cpu;
