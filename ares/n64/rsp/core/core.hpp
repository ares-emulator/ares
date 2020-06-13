//{
  //core.cpp
  auto instruction() -> void;
  auto instructionEpilogue() -> bool;
  auto instructionDebug() -> void;
  auto powerCore() -> void;

  //decoder.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderCOP0() -> void;
  auto decoderCOP2() -> void;
  auto decoderLWC2() -> void;
  auto decoderSWC2() -> void;

  auto instructionINVALID() -> void;

  //serialization.cpp
  auto serializeCore(serializer&) -> void;

  #include "pipeline.hpp"
  #include "cpu.hpp"
  #include "scc.hpp"
  #include "vu.hpp"
  #include "recompiler.hpp"
  #include "disassembler.hpp"
//};
