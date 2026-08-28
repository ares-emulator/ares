struct Harmony : ARM7TDMI, Thread {
  Node::Object node;

  enum class Access : u32 { Unmapped, Granted, Fault };

  struct Invocation {
    u32 pc = 0;
    u32 lr = 0;
    u32 sp = 0;

    explicit operator bool() const { return pc && sp; }
  };

  //harmony.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;
  auto call(bool irqDrivenAudio = false) -> bool;
  auto resetProcessor() -> bool;
  auto handleTrap(u32 address) -> bool;

  //timing.cpp
  auto step(u32 clocks) -> void override;

  //memory.cpp
  auto sleep() -> void override;
  auto get(u32 mode, n32 address) -> n32 override;
  auto set(u32 mode, n32 address, n32 word) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  u64 runs = 0;
  u32 timer1Control = 0;
  u32 timer1Counter = 0;
  u32 systickControl = 4;
  u32 systickReload = 0;
  u32 systickCounter = 0;
  u32 systickCalibration = 0x00abcdef;
  u32 mamControl = 0;
  u32 callCycles = 0;
  u32 instructions = 0;
  bool running = false;
  bool irqDrivenAudio = false;
  bool callPending = false;
  bool pendingIrqDrivenAudio = false;
  bool faulted = false;
};

extern Harmony harmony;
