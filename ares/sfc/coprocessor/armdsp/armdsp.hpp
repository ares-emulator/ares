//ARMv3 (ARM60)

//note: this coprocessor uses the ARMv4 (ARM7TDMI) core as its base
//instruction execution forces ARM mode to remove ARMv4 THUMB access
//there is a possibility the ARMv3 supports 26-bit mode; but cannot be verified

struct ArmDSP : ARM7TDMI, Thread {
  Node::Component node;
  Node::Instruction eventInstruction;
  uint Frequency;
  #include "registers.hpp"

  //armdsp.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;

  auto boot() -> void;
  auto main() -> void;
  auto step(uint clocks) -> void override;
  auto power() -> void;
  auto reset() -> void;  //soft reset

  //memory.cpp
  auto sleep() -> void override;
  auto get(uint mode, uint32 address) -> uint32 override;
  auto set(uint mode, uint32 address, uint32 word) -> void override;

  //io.cpp
  auto read(uint24 address, uint8 data) -> uint8;
  auto write(uint24 address, uint8 data) -> void;

  //serialization.cpp
  auto firmware() const -> vector<uint8>;
  auto serialize(serializer&) -> void;

  uint8 programROM[128 * 1024];
  uint8 dataROM[32 * 1024];
  uint8 programRAM[16 * 1024];
};

extern ArmDSP armdsp;
