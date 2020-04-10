struct CPU : ARM7TDMI, Thread, IO {
  Node::Component node;
  Memory::Writable<uint8> iwram;  // 32KB
  Memory::Writable<uint8> ewram;  //256KB

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view type) -> void;

    struct Memory {
      Node::Memory iwram;
      Node::Memory ewram;
    } memory;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification interrupt;  //todo: ARM7TDMI needs to notify CPU when interrupts occur
    } tracer;
  } debugger;

  struct Interrupt { enum : uint {
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

  auto clock() const -> uint { return context.clock; }
  auto halted() const -> bool { return context.halted; }
  auto stopped() const -> bool { return context.stopped; }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void override;
  auto power() -> void;

  //prefetch.cpp
  auto prefetchSync(uint32 addr) -> void;
  auto prefetchStep(uint clocks) -> void;
  auto prefetchWait() -> void;
  auto prefetchRead() -> uint16;

  //bus.cpp
  auto sleep() -> void override;
  auto get(uint mode, uint32 addr) -> uint32 override;
  auto set(uint mode, uint32 addr, uint32 word) -> void override;
  auto _wait(uint mode, uint32 addr) -> uint;

  //io.cpp
  auto readIO(uint32 addr) -> uint8;
  auto writeIO(uint32 addr, uint8 byte) -> void;

  auto readIWRAM(uint mode, uint32 addr) -> uint32;
  auto writeIWRAM(uint mode, uint32 addr, uint32 word) -> void;

  auto readEWRAM(uint mode, uint32 addr) -> uint32;
  auto writeEWRAM(uint mode, uint32 addr, uint32 word) -> void;

  //dma.cpp
  auto dmaVblank() -> void;
  auto dmaHblank() -> void;
  auto dmaHDMA() -> void;

  //timer.cpp
  auto runFIFO(uint n) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct uintVN {
    auto operator()() const -> uint32 { return data & mask; }
    auto setBits(uint bits) -> void { mask = (1 << bits) - 1; }

    uint32 data;
    uint32 mask;
  };

  struct DMA {
    //dma.cpp
    auto run() -> bool;
    auto transfer() -> void;

     uint2 id;

     uint1 active;
     int32 waiting;

     uint2 targetMode;
     uint2 sourceMode;
     uint1 repeat;
     uint1 size;
     uint1 drq;
     uint2 timingMode;
     uint1 irq;
     uint1 enable;
    uint32 data;

    uintVN source;
    uintVN target;
    uintVN length;

    struct Latch {
      uintVN source;
      uintVN target;
      uintVN length;
    } latch;
  } dma[4];

  struct Timer {
    //timer.cpp
    auto run() -> void;
    auto step() -> void;

     uint2 id;

     uint1 pending;

    uint16 period;
    uint16 reload;

     uint2 frequency;
     uint1 cascade;
     uint1 irq;
     uint1 enable;
  } timer[4];

  struct Serial {
     uint1 shiftClockSelect;
     uint1 shiftClockFrequency;
     uint1 transferEnableReceive;
     uint1 transferEnableSend;
     uint1 startBit;
     uint1 transferLength;
     uint1 irqEnable;

    uint16 data[4];
     uint8 data8;
  } serial;

  struct Keypad {
    //keypad.cpp
    auto run() -> void;

    uint1 enable;
    uint1 condition;
    uint1 flag[10];
  } keypad;

  struct Joybus {
     uint1 sc;
     uint1 sd;
     uint1 si;
     uint1 so;
     uint1 scMode;
     uint1 sdMode;
     uint1 siMode;
     uint1 soMode;
     uint1 siIRQEnable;
     uint2 mode;

     uint1 resetSignal;
     uint1 receiveComplete;
     uint1 sendComplete;
     uint1 resetIRQEnable;

    uint32 receive;
    uint32 transmit;

     uint1 receiveFlag;
     uint1 sendFlag;
     uint2 generalFlag;
  } joybus;

  struct IRQ {
     uint1 ime;
    uint16 enable;
    uint16 flag;
  } irq;

  struct Wait {
    uint2 nwait[4];
    uint1 swait[4];
    uint2 phi;
    uint1 prefetch;
    uint1 gameType;
  } wait;

  struct Memory {
    uint1 disable;
    uint3 unknown1;
    uint1 ewram = 1;
    uint4 ewramWait = 13;
    uint4 unknown2;
  } memory;

  struct {
    auto empty() const { return addr == load; }
    auto full() const { return load - addr == 16; }

    uint16 slot[8];
    uint32 addr;       //read location of slot buffer
    uint32 load;       //write location of slot buffer
     int32 wait = 1;  //number of clocks before next slot load
  } prefetch;

  struct Context {
    uint32 clock;
     uint1 halted;
     uint1 stopped;
     uint1 booted;  //set to true by the GBA BIOS
     uint1 dmaActive;
  } context;
};

extern CPU cpu;
