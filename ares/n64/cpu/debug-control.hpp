//Adapted from ares-64 1c9b9637476a83f7b80d3db1115a0f71529820d9.
//Bounded stack inspection is host-side; the private call gate is host-only state.
//CPU debug engine for external debuggers (the MCP/telemetry frontend).
//Included inside struct CPU (cpu.hpp); implementation in debug-control.cpp.
//
//Independent of the GDB stub: GDB's hooks are gated on an attached GDB client and
//report through GDB packets, while this engine keeps its own breakpoint/watchpoint
//tables, execution gate and immutable stop snapshot.
//
//Execution model:
//  * While `active`, CPU::instruction() runs the interpreter (never the JIT), so every
//    instruction passes beforeInstruction()/afterInstruction() and every CPU data
//    access passes the watch hooks in CPU::read/CPU::write.
//  * A stop never spins inside the core: it sets `halted`, which makes CPU::main()
//    return to the frontend at an instruction boundary. The frontend keeps the
//    machine stopped by not calling run() again, and resumes with resume().
//  * Breakpoints stop *before* the instruction executes (no pipeline mutation).
//    Watchpoints latch the first matching access, let the instruction complete, then
//    stop before the next instruction (post-access semantics).
//  * No JSON, paths, sockets or frontend references live here: the frontend reads the
//    plain structs below and serializes them.
struct DebugControl {
  CPU& self;
  DebugControl(CPU& self) : self(self) {}

  enum class StopReason : u32 { None, Breakpoint, Watchpoint, InstructionsDone, Requested, SliceBudget };
  enum class Field : u32 { Pc, Gpr, Hi, Lo, Value };
  enum class Op : u32 { Eq, Ne, Lt, Le, Gt, Ge };
  enum : u32 { AccessRead = 1, AccessWrite = 2 };
  static constexpr u32 MaxPredicates = 8;
  static constexpr u32 MaxBreakpoints = 256;
  static constexpr u32 MaxWatchpoints = 64;

  //One `field op value` comparison, unsigned, after masking the field with `mask`.
  struct Predicate {
    Field field = Field::Pc;
    u32 index = 0;  //GPR number for Field::Gpr
    Op op = Op::Eq;
    u64 value = 0;
    u64 mask = ~0ull;
  };
  //All predicates must match (empty = unconditional).
  struct Condition {
    std::vector<Predicate> all;
  };

  struct Breakpoint {
    u32 id = 0;
    u64 address = 0;  //as supplied (normalized to 64-bit sign-extended VA when it was a VA)
    u32 paddr = 0;    //physical RDRAM address
    Condition condition;
    u64 hits = 0;
  };

  struct Watchpoint {
    u32 id = 0;
    u64 address = 0;
    u32 paddr = 0;
    u32 length = 0;
    u32 kind = AccessWrite;  //AccessRead | AccessWrite
    Condition condition;
    u64 hits = 0;
  };

  //A matching CPU data access (first match of the instruction).
  struct AccessHit {
    bool valid = false;
    u32 watchId = 0;
    std::vector<u32> otherIds;  //other watchpoints that also matched in this instruction
    u32 kind = 0;               //AccessRead or AccessWrite
    u64 vaddr = 0;              //effective virtual address of the semantic access
    u32 paddr = 0;
    u32 size = 0;               //bytes
    u64 value = 0;              //read value / stored bytes (big-endian, `size` bytes)
    u64 oldValue = 0;           //previous bytes (writes, side-effect-free CPU-view read)
    bool hasOld = false;
    u64 pc = 0;                 //PC of the accessing instruction
    u32 instruction = 0;
  };

  //Immutable copy of the architectural state, taken when the stop happened.
  struct Snapshot {
    bool valid = false;
    u64 gpr[32] = {};
    u64 lo = 0, hi = 0;
    u64 pc = 0;          //next instruction to execute
    u64 nextPc = 0;      //instruction after that (branch target when in a delay slot)
    bool delaySlot = false;  //`pc` is a branch delay slot
    u64 cop0[32] = {};
    u32 cop0Valid = 0;   //bit n set when cop0[n] could be read without side effects
    u64 fpr[32] = {};
    u32 fcr0 = 0, fcr31 = 0;
  };

  struct Stop {
    StopReason reason = StopReason::None;
    u32 breakpointId = 0;
    std::vector<u32> otherBreakpointIds;
    u64 breakpointPc = 0;
    AccessHit access;
    Snapshot snapshot;
    u64 frame = 0;  //epoch identity belongs to the host operation
    u64 instructions = 0;  //instructionsRetired when the stop happened
  };

  //call trace record (JAL/JALR/branch-and-link), decoded before execution
  struct CallRecord {
    u64 sequence = 0;
    u64 frame = 0;
    u64 pc = 0;          //call site
    u64 target = 0;
    u32 kind = 0;        //0=jal 1=jalr 2=branch-and-link(taken) 3=jalr rd=0 (jump, not a call)
    u32 linkRegister = 0;
    u64 linkValue = 0;
    u64 sp = 0;
    u64 registers[32] = {};
    u64 f12 = 0;
    bool fr = false;
  };

  //PC histogram entry
  struct PcCount {
    u64 pc;
    u64 frame;
    u64 count;
  };

  //debug-control.cpp
  auto power() -> void;
  auto normalizeAddress(u64 address, u32 length, u32& paddr, string& error) const -> bool;
  auto addBreakpoint(u64 address, Condition condition, string& error) -> u32;
  auto removeBreakpoint(u32 id) -> bool;
  auto addWatchpoint(u64 address, u32 length, u32 kind, Condition condition, string& error) -> u32;
  auto removeWatchpoint(u32 id) -> bool;
  auto clearHits() -> void;

  //execution gate
  auto updateActive() -> void;
  auto requestStepInstructions(u64 count) -> void;
  auto requestHalt() -> void;
  auto resume() -> void;
  auto setSliceBudget(u64 instructions) -> void { sliceBudget = instructions; sliceCount = 0; updateActive(); }
  auto exitMain() -> bool;

  //hooks (called from cpu.cpp / memory.cpp / interpreter-ipu.cpp)
  auto beforeInstruction(PhysAccess access) -> bool;  //true = stop before this instruction
  auto afterInstruction(u64 pc, u32 instruction) -> void;
  auto semanticRead(u64 vaddr, u32 length) -> void { narrowVaddr = vaddr; narrowLength = length; }
  template<u32 Size> auto onRead(const PhysAccess& access, u64 value) -> void;
  template<u32 Size> auto onWrite(const PhysAccess& access, u64 data) -> void;

  auto evaluate(const Condition&, const AccessHit* access) const -> bool;
  auto snapshot() const -> Snapshot;
  auto controlRegister(u32 index, u64& value) const -> bool;
  auto takeStop(StopReason reason) -> void;

  //tracing
  auto beginTrace(bool pcs, bool calls, u64 startFrame, u64 endFrame, u64 lowPc, u64 highPc, u64 maxRecords) -> void;
  auto endTrace() -> void;
  auto flushPcBucket() -> void;

  struct CallGate {
    bool active = false, returned = false, invalidReturn = false, exception = false;
    bool returnPending = false, armed = false;
    u64 pc = 0, sp = 0, delayPc = 0;
    u32 exceptionCode = 0;
  } callGate;
  bool active = false;   //any per-instruction hook required (gates the interpreter)
  bool watchActive = false;
  bool gate = false;     //CPU::main must consult exitMain() (active, halted or slice budget)
  bool halted = false;   //stop latched; CPU::main returns until resume()
  Stop stop;             //valid while halted (and after, until the next stop)
  bool stopValid = false;

  u64 instructionsRetired = 0;  //counted while active (debug coordinate, not serialized)
  u64 stepRemaining = 0;
  bool stepping = false;
  bool haltRequested = false;
  bool suppressBreakOnce = false;
  u64 suppressPc = 0;
  u64 sliceBudget = 0;   //0 = unlimited; else CPU::main yields after this many iterations
  u64 sliceCount = 0;

  std::vector<Breakpoint> breakpoints;
  std::vector<Watchpoint> watchpoints;
  u32 nextBreakpointId = 1;
  u32 nextWatchpointId = 1;

  //current instruction
  u64 currentPc = 0;
  u32 currentInstruction = 0;
  AccessHit pendingAccess;
  u64 narrowVaddr = 0;
  u32 narrowLength = 0;

  //tracing state
  struct Trace {
    bool pcs = false;
    bool calls = false;
    u64 startFrame = 0, endFrame = 0;  //[start, end) in VI frames of the current epoch
    u64 lowPc = 0, highPc = ~0ull;     //half-open filter on the executing PC (64-bit VA)
    u64 maxRecords = 0;
    bool truncated = false;
    u64 droppedRecords = 0;
    u64 overflowObservations = 0;  //unrepresented non-RDRAM PCs when the map is full
    u64 totalInstructions = 0;         //traced instructions (within the frame interval + filter)
    u64 bucketFrame = ~0ull;
    std::vector<u32> counters;         //indexed by physical RDRAM word address
    std::vector<u32> touched;          //indices with nonzero counters
    std::vector<u64> vaddrs;           //VA observed for each touched physical word
    std::unordered_map<u64, u64> other;  //non-RDRAM PCs
    std::vector<PcCount> pcRecords;
    std::vector<CallRecord> callRecords;
    u64 callSequence = 0;
    u64 instructionsObserved = 0, callsObserved = 0, excludedCallsite = 0, excludedTarget = 0;
    u64 targetLow = 0, targetHigh = ~0ull;
    u32 registerMask = 0;
    bool captureF12 = false;
  } trace;
};
