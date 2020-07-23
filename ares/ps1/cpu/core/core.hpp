//{
  //core.cpp
  auto raiseException(uint, uint = 0) -> void;
  auto instruction() -> void;
  auto instructionEpilogue() -> bool;
  auto instructionHook() -> void;
  auto instructionDebug() -> void;
  auto powerCore(bool reset) -> void;

  //memory.cpp
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;

  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //decoder.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderSCC() -> void;
  auto decoderGTE() -> void;

  auto instructionCOP1() -> void;
  auto instructionCOP3() -> void;
  auto instructionINVALID() -> void;

  //serialization.cpp
  auto serializeCore(serializer&) -> void;

  #include "exception.hpp"
  #include "cpu.hpp"
  #include "scc.hpp"
  #include "gte.hpp"
  #include "disassembler.hpp"

  struct Pipeline {
    u32 address;
    u32 instruction;
  } pipeline;

  static constexpr bool Endian = 0;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 3 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 3 : 0);
//};
