//{
  //core.cpp
  auto raiseException(uint, uint = 0) -> void;
  auto instruction() -> void;
  auto instructionEpilogue() -> bool;
  auto instructionDebug() -> void;
  auto powerR4300(bool reset) -> void;

  //memory.cpp
  auto readAddress (u32 address) -> maybe<u32>;
  auto writeAddress(u32 address) -> maybe<u32>;

  auto readByte    (u32 address) -> maybe<u32>;
  auto readHalf    (u32 address) -> maybe<u32>;
  auto readWord    (u32 address) -> maybe<u32>;
  auto readDouble  (u32 address) -> maybe<u64>;

  auto writeByte   (u32 address,  u8 data) -> bool;
  auto writeHalf   (u32 address, u16 data) -> bool;
  auto writeWord   (u32 address, u32 data) -> bool;
  auto writeDouble (u32 address, u64 data) -> bool;

  //decoder.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderCOP0() -> void;
  auto decoderCOP1() -> void;

  auto instructionCOP2() -> void;
  auto instructionCOP3() -> void;
  auto instructionINVALID() -> void;

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

  struct Context {
    CPU& self;
    Context(CPU& self) : self(self) {}

    enum Mode : uint { Kernel, Supervisor, User };
    enum Segment : uint { Invalid, Mapped, Cached, Uncached };
    uint mode;
    uint segment[8];  //512_MiB chunks

    //core.cpp
    auto setMode() -> void;
  } context{*this};

  #include "exception.hpp"
  #include "pipeline.hpp"
  #include "cpu.hpp"
  #include "scc.hpp"
  #include "fpu.hpp"
  #include "recompiler.hpp"
  #include "disassembler.hpp"

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);
//};
