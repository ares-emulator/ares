//{
  //core.cpp
  auto raiseException(uint, uint = 0) -> void;
  auto instruction() -> void;
  auto instructionEpilogue() -> bool;
  auto instructionDebug() -> void;
  auto powerCore(bool reset) -> void;

  //memory.cpp
  auto readAddress (u32 address) -> maybe<u32>;
  auto writeAddress(u32 address) -> maybe<u32>;

  auto readByte(u32 address) -> maybe< u8>;
  auto readHalf(u32 address) -> maybe<u16>;
  auto readWord(u32 address) -> maybe<u32>;

  auto writeByte(u32 address,  u8 data) -> bool;
  auto writeHalf(u32 address, u16 data) -> bool;
  auto writeWord(u32 address, u32 data) -> bool;

  //decoder.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderCOP0() -> void;
  auto decoderCOP2() -> void;

  auto instructionCOP1() -> void;
  auto instructionCOP3() -> void;
  auto instructionINVALID() -> void;

  //serialization.cpp
  auto serializeCore(serializer&) -> void;

  #include "exception.hpp"
  #include "cpu.hpp"
  #include "scc.hpp"
  #include "gte.hpp"

  static constexpr bool Endian = 0;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 3 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 3 : 0);
//};
