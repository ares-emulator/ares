//{
  //core.cpp
  auto instruction() -> void;
  auto instructionDEBUG() -> void;
  auto powerCore() -> void;

  //decoder.cpp
  auto instructionEXECUTE() -> void;
  auto instructionSPECIAL() -> void;
  auto instructionREGIMM() -> void;
  auto instructionCOP0() -> void;
  auto instructionCOP2() -> void;
  auto instructionLWC2() -> void;
  auto instructionSWC2() -> void;

  //serialization.cpp
  auto serializeCore(serializer&) -> void;

  #include "pipeline.hpp"
  #include "cpu.hpp"
  #include "scc.hpp"
  #include "vu.hpp"
  #include "disassembler.hpp"
//};
