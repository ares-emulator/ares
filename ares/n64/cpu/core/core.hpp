//{
  //core.cpp
  auto powerR4300() -> void;

  //memory.cpp
  auto readByte   (u32 address) -> maybe<u32>;
  auto readHalf   (u32 address) -> maybe<u32>;
  auto readWord   (u32 address) -> maybe<u32>;
  auto readDouble (u32 address) -> maybe<u64>;
  auto writeByte  (u32 address,  u8 data) -> void;
  auto writeHalf  (u32 address, u16 data) -> void;
  auto writeWord  (u32 address, u32 data) -> void;
  auto writeDouble(u32 address, u64 data) -> void;

  #include "instruction.hpp"

  union r64 {
    struct {   int32_t order_msb2(i32h, i32); };
    struct {  uint32_t order_msb2(u32h, u32); };
    struct { float32_t order_msb2(f32h, f32); };
    struct {   int64_t i64; };
    struct {  uint64_t u64; };
    struct { float64_t f64; };
    auto i128() const ->  int128_t { return  (int128_t)i64; }
    auto u128() const -> uint128_t { return (uint128_t)u64; }
  };

  //scc-registers.cpp
  auto getControlRegister(uint5) -> u64;
  auto setControlRegister(uint5, uint64) -> void;

  struct Exception {
    CPU& self;
    auto interruptRCP() { self.raiseException(0, 0, 1 << 2); }
    auto interruptDD() { self.raiseException(0, 0, 1 << 3); }
    auto interruptCompare() { self.raiseException(0, 0, 1 << 7); }
    auto tlbModification() { self.raiseException(1); }
    auto tlbLoad() { self.raiseException(2); }
    auto tlbStore() { self.raiseException(3); }
    auto addressLoad() { self.raiseException(4); }
    auto addressStore() { self.raiseException(5); }
    auto busInstruction() { self.raiseException(6); }
    auto busData() { self.raiseException(7); }
    auto systemCall() { self.raiseException(8); }
    auto breakpoint() { self.raiseException(9); }
    auto reservedInstruction() { self.raiseException(10); }
    auto coprocessor0() { self.raiseException(11, 0); }
    auto coprocessor1() { self.raiseException(11, 1); }
    auto coprocessor2() { self.raiseException(11, 2); }
    auto coprocessor3() { self.raiseException(11, 3); }
    auto arithmeticOverflow() { self.raiseException(12); }
    auto trap() { self.raiseException(13); }
    auto floatingPoint() { self.raiseException(15); }
    auto watchAddress() { self.raiseException(23); }
  } exception{*this};

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

  #include "pipeline.hpp"

  struct Core {
    enum Register : uint {
      R0,                              //zero (read-only)
      AT,                              //assembler temporary
      V0, V1,                          //arithmetic values
      A0, A1, A2, A3,                  //subroutine parameters
      T0, T1, T2, T3, T4, T5, T6, T7,  //temporary registers
      S0, S1, S2, S3, S4, S5, S6, S7,  //saved registers
      T8, T9,                          //temporary registers
      K0, K1,                          //kernel registers
      GP,                              //global pointer
      SP,                              //stack pointer
      S8,                              //saved register
      RA,                              //return address
    };

    r64 r[32];
    r64 lo;
    r64 hi;
    u64 pc;  //program counter

    //internal
    maybe<u64> ip;
  } core;

  #include "scc-registers.hpp"

  struct FPU {
    r64 r[32];
    u32 cr[32];
    bool cf[8];
  } fpu;

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);

  #include "disassembler.hpp"
//};
