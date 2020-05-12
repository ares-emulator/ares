//{
  //core.cpp
  auto powerR4300() -> void;

  //memory.cpp
  auto readByte   (u64 address) -> maybe<u32>;
  auto readHalf   (u64 address) -> maybe<u32>;
  auto readWord   (u64 address) -> maybe<u32>;
  auto readDouble (u64 address) -> maybe<u64>;
  auto writeByte  (u64 address,  u8 data) -> void;
  auto writeHalf  (u64 address, u16 data) -> void;
  auto writeWord  (u64 address, u32 data) -> void;
  auto writeDouble(u64 address, u64 data) -> void;

  enum Interrupt : uint {
    Software0 = 0,
    Software1 = 1,
    RCP       = 2,
    Cartridge = 3,
    Reset     = 4,
    ReadRDB   = 5,
    WriteRDB  = 6,
    Timer     = 7,
  };

  //serialization.cpp
  auto serializeR4300(serializer&) -> void;

  #include "tlb.hpp"
  #include "instruction.hpp"
  #include "exception.hpp"
  #include "pipeline.hpp"
  #include "cpu-registers.hpp"
  #include "scc-registers.hpp"
  #include "fpu-registers.hpp"
  #include "disassembler.hpp"

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);
//};
