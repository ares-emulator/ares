//ARMv3 (ARM60)

//note: this coprocessor uses the ARMv4 (ARM7TDMI) core as its base
//instruction execution forces ARM mode to remove ARMv4 THUMB access
//there is a possibility the ARMv3 supports 26-bit mode; but cannot be verified

struct ARMDSP : ARM7TDMI, Thread {
  Node::Object node;
  n32 Frequency;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
    } tracer;
  } debugger;

  //armdsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto boot() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void override;
  auto power() -> void;
  auto reset() -> void;  //soft reset

  //memory.cpp
  auto sleep() -> void override;
  auto get(u32 mode, n32 address) -> n32 override;
  auto set(u32 mode, n32 address, n32 word) -> void override;

  //io.cpp
  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  //serialization.cpp
  auto firmware() const -> vector<n8>;
  auto serialize(serializer&) -> void;

  n8 programROM[128_KiB];
  n8 dataROM[32_KiB];
  n8 programRAM[16_KiB];

  struct Bridge {
    struct Buffer {
      n1 ready;
      n8 data;
    };
    Buffer cputoarm;
    Buffer armtocpu;
    n32 timer;
    n32 timerlatch;
    n1  reset;
    n1  ready;
    n1  signal;

    auto status() const -> n8 {
      n8 data;
      data.bit(0) = armtocpu.ready;
      data.bit(2) = signal;
      data.bit(3) = cputoarm.ready;
      data.bit(7) = ready;
      return data;
    }
  } bridge;
};

extern ARMDSP armdsp;
