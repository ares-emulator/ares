//{
  //core.cpp
  auto powerCore() -> void;

  //memory.cpp
  auto readByte(u32 address) -> maybe<u32>;
  auto readHalf(u32 address) -> maybe<u32>;
  auto readWord(u32 address) -> maybe<u32>;
  auto writeByte(u32 address,  u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //instruction.cpp
  auto instruction() -> void;
  auto instructionDEBUG() -> void;
  auto instructionEXECUTE() -> void;
  auto instructionSPECIAL() -> void;
  auto instructionREGIMM() -> void;
  auto instructionCOP0() -> void;
  auto instructionCOP2() -> void;

  //serialization.cpp
  auto serializeCore(serializer&) -> void;

  #include "pipeline.hpp"
  #include "cpu.hpp"
  #include "scc.hpp"
  #include "vu.hpp"
  #include "disassembler.hpp"
//};
