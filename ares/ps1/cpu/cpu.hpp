//LSI CoreWare CW33300 (MIPS R3000A core)

struct CPU : Thread {
  Node::Object node;
  Memory::Writable ram;
  Memory::Writable scratchpad;
  Memory::Readable exe;

  #include "debugger/debugger.hpp"

  using cs32 = const s32;
  using cu32 = const u32;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto synchronize() -> void;
  auto ioSynchronize() -> void;
  auto waitBus(u8 owner) -> void;
  auto stepBus(u32 clocks, u8 owner = Bus::CPUAccess, bool ramData = false) -> void;

  //instruction.cpp
  alwaysinline auto instruction() -> void;
  auto instructionPrologue(u32 instruction) -> void;
  auto instructionEpilogue() -> void;

  //cpu.cpp
  auto instructionHook() -> void;

  auto power(bool reset) -> void;

  n1 exeLoaded = 0;

  u32 accruedCycles = 0;
  s32 cyclesUntilForcedSync = 0;
  const s32 forceSyncInterval = 1024;
  const s32 branchCooldownCycles = 512;
  const s32 ioCooldownCycles = 128;

  struct Execution {
    enum : u8 {
      EventNone,
      EventGTEDataWrite,
      EventGTEControlWrite,
    };

    u64 clock = 0;

    struct MultiplyDivide {
      bool active = false;
      u64 completion = 0;
      u32 hi = 0;
      u32 lo = 0;
    } multiplyDivide;

    u64 gteCompletion = 0;
    u64 nextEventSequence = 0;

    struct Event {
      u8 type = EventNone;
      u8 index = 0;
      u32 value = 0;
      u64 completion = 0;
      u64 sequence = 0;
    } events[8];

    u64 retiredInstructions = 0;
    struct StatusVisibility {
      bool managed = false;
      u32 visible = 0;
      u64 nextSequence = 0;

      struct Event {
        bool active = false;
        u32 value = 0;
        u64 retirement = 0;
        u64 sequence = 0;
      } events[4];
    } status;
  } execution;

  //execution.cpp
  auto retireExecutionUnits() -> void;
  auto stallMultiplyDivide() -> void;
  auto scheduleMultiplyDivide(u32 hi, u32 lo, u32 cycles) -> void;
  auto multiplyCycles(s32 rs) const -> u32;
  auto multiplyCycles(u32 rs) const -> u32;
  auto stallGTE() -> void;
  auto scheduleGTE(u32 cycles) -> void;
  auto gteCommandCycles(u8 command) const -> u32;
  auto scheduleGTEDataWrite(u8 index, u32 value) -> void;
  auto scheduleGTEControlWrite(u8 index, u32 value) -> void;
  auto scheduleExecutionEvent(u8 type, u8 index, u32 value, u32 delaySlots) -> void;
  auto scheduleStatusVisibility(u32 previous, u32 value, u64 retirement) -> void;
  auto retireStatusVisibility() -> void;
  auto latestStatusVisibilityRetirement() const -> u64;
  auto synchronizeStatusVisibility() -> void;
  auto coprocessor2Enabled() const -> bool;

  struct MemoryFrontend {
    enum : u8 {
      BusWaitNone,
      BusWaitDCacheScratchpad,
      BusWaitInvalidInstructionBlockSize,
    };

    struct InstructionRefill {
      bool active = false;
      u32 address = 0;
      u32 tag = 0;
      u8 line = 0;
      u8 requestedWord = 0;
      u8 nextWord = 0;
      u8 finalWord = 0;
      u8 completedWords = 0;
      u64 started = 0;
      u64 completion = 0;
      bool granted = false;
    } refill;

    struct WriteBuffer {
      struct Entry {
        u32 address = 0;
        u32 offset = 0;
        u32 data = 0;
        u8 byteEnable = 0;
        u64 sequence = 0;
      } entries[4];

      u8 head = 0;
      u8 count = 0;
      u64 completion = 0;
      u64 nextSequence = 0;
      u64 ready = 0;
      u8 delay = 4;
    } writeBuffer;

    u8 busWait = BusWaitNone;
  } frontend;

  //write-buffer.cpp
  auto retireMemoryFrontend() -> void;
  auto scheduleWriteBuffer() -> bool;
  auto enterBusWait(u8 reason) -> void;
  auto writeBufferByteEnable(u32 address, u32 size) const -> u8;
  auto enqueueWriteBuffer(u32 address, u32 offset, u32 data, u8 byteEnable) -> void;
  auto retireWriteBuffer() -> void;
  auto waitWriteBuffer(u32 offset, u8 byteEnable) -> void;

  struct Pipeline {
    u32 address = 0;
    n32 instruction = 0;
  } pipeline;

  //delay-slots.cpp
  struct Delay {
    //load delay slots
    struct {
      u32* target = nullptr;
      u32  source = 0;
    } load[2];

    //branch delay slots
    struct Branch {
      n1  slot;
      n1  take;
      n32 address;
    } branch[2];

  } delay;

  auto load(u32& target) const -> u32;
  auto load(u32& target, u32 source) -> void;
  auto store(u32& target, u32 source) -> void;
  template<u32 N = 1> auto branch(u32 address, bool take = true) -> void;
  auto processDelayLoad() -> void;
  auto processDelayBranch() -> void;

  //memory.cpp
  auto fetch(u32 address) -> u32;
  auto peek(u32 address) -> u32;

  template<u32 Size> auto read(u32 address) -> u32;
  template<u32 Size> auto write(u32 address, u32 data) -> void;

  template<u32 Size> auto readRAM(u32 address) -> u32;
  template<u32 Size> auto writeRAM(u32 address, u32 data) -> void;

  //icache.cpp
  struct InstructionCache {
    auto fetch(u32 address) -> u32;
    auto read(u32 address) -> u32;
    auto write(u32 address, u32 data, u8 byteEnable) -> void;
    auto readTag(u32 address) -> u32;
    auto writeTag(u32 address, u32 data) -> void;
    auto startRefill(u32 address) -> bool;
    auto retireRefill() -> void;
    auto refillLatency(u32 words) const -> u32;
    auto power(bool reset) -> void;

    //4KB
    struct Line {
      u32 words[4];
      u32 tag = 0;
      u8 valid = 0;
    } lines[256];
  } icache;

  //exceptions.cpp
  struct Exception {
    CPU& self;
    Exception(CPU& self) : self(self) {}

    auto operator()() -> bool;
    auto trigger(u32 code, u32 coprocessor = 0) -> void;

    auto interruptsPending() const -> u8;
    auto interrupt() -> void;
    template<u32 Mode> auto address(u32 address) -> void;
    auto busInstruction() -> void;
    auto busData() -> void;
    auto systemCall() -> void;
    auto breakpoint(bool overrideVectorLocation) -> void;
    auto reservedInstruction() -> void;
    auto coprocessor(u32 coprocessor) -> void;
    auto arithmeticOverflow() -> void;
    auto trap() -> void;

    bool triggered;
  } exception{*this};

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IPU {
    u32 r[32];
    u32 lo;
    u32 hi;
    u32 pb;  //previous PC
    u32 pc;  //current PC
    u32 pd;  //next PC
  } ipu;

  #include "scc/scc.hpp"
  #include "gte/gte.hpp"
  #include "interpreter/interpreter.hpp"
};

extern CPU cpu;
