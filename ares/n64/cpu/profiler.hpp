// Bounded emulation-thread profiler; addresses retain the reference 32-bit convention.
struct Profiler {
  struct FuncStat {
    u32 addr = 0;
    u64 callCount = 0, inclCycles = 0, exclCycles = 0, waitCycles = 0;
    u64 inclBytesIn = 0, inclBytesOut = 0, exclBytesIn = 0, exclBytesOut = 0;
    u64 inclCacheBytes[3] = {}, exclCacheBytes[3] = {};
    bool isSpin = false;
  };
  struct Frame {
    u32 funcAddr = 0, retAddr = 0, sp = 0;
    u64 entryCycle = 0, segmentCycle = 0;
    bool newCall = true, isException = false;
    u64 childCycles = 0, childWait = 0;
    u64 bytesInOwn = 0, bytesOutOwn = 0, childBytesIn = 0, childBytesOut = 0;
    u64 cacheOwn[3] = {}, childCache[3] = {};
  };
  struct Suspended { u32 lo = 0, hi = 0; u64 used = 0; std::vector<Frame> frames; };
  bool enabled = false;
  std::unordered_map<u32, FuncStat> stats, frameStats, frameAccum;
  std::vector<Frame> callStack;
  std::vector<Suspended> suspended;
  u32 spLo = 0, spHi = 0, excActive = 0;
  bool haveSp = false;
  u64 stackClock = 0, frameCount = 0, droppedFunctionUpdates = 0, droppedStackFrames = 0;
  static constexpr u32 regionSlack = 0x8000, maxStacks = 16, maxStackDepth = 1024;
  static constexpr u32 maxFrames = 1000, maxFunctions = 65536, CacheKinds = 3;
  static constexpr u32 kExcBase = 0xe0000000;
  auto isExceptionAddr(u32 addr) const -> bool { return (addr & 0xffff0000) == kExcBase; }
  auto exceptionEntryAddr(u32 code) -> u32;
  auto labelFor(u32 addr) -> string;
  auto now() -> u64;
  auto onInstruction(u64 address, u32 instruction) -> void;
  auto onBusAccess(bool toRDRAM, u64 bytes) -> void;
  auto commitFrame(Frame& frame, Frame* parent, u64 time) -> void;
  auto flushFrames() -> void;
  auto popFrame() -> bool;
  auto switchStack(u32 sp) -> void;
  auto onException(u32 code) -> void;
  auto onEret() -> void;
  auto onFrame() -> void;
  auto setEnabled(bool value) -> void;
  auto power() -> void;
  auto clearStats() -> void;
} profiler;
