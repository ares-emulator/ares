//{
  //core.cpp
  auto powerCore() -> void;

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
