//{
  //fpu-registers.cpp
  template<typename T> auto fgr(uint) -> T&;
  auto getControlRegisterFPU(uint5) -> u32;
  auto setControlRegisterFPU(uint5, uint32) -> void;

  union fpu64 {
    struct {   int32_t order_msb2(i32h, i32); };
    struct {  uint32_t order_msb2(u32h, u32); };
    struct { float32_t order_msb2(f32h, f32); };
    struct {   int64_t i64; };
    struct {  uint64_t u64; };
    struct { float64_t f64; };
  };

  struct FPU {
    fpu64 r[32];

    struct Coprocessor {
      static constexpr u8 revision = 0x00;
      static constexpr u8 implementation = 0x0b;
    } coprocessor;

    struct ControlStatus {
      uint2 roundMode = 0;
      struct Flag {
        uint1 inexact = 0;
        uint1 underflow = 0;
        uint1 overflow = 0;
        uint1 divisionByZero = 0;
        uint1 invalidOperation = 0;
      } flag;
      struct Enable {
        uint1 inexact = 0;
        uint1 underflow = 0;
        uint1 overflow = 0;
        uint1 divisionByZero = 0;
        uint1 invalidOperation = 0;
      } enable;
      struct Cause {
        uint1 inexact = 0;
        uint1 underflow = 0;
        uint1 overflow = 0;
        uint1 divisionByZero = 0;
        uint1 invalidOperation = 0;
        uint1 unimplementedOperation = 0;
      } cause;
      uint1 compare = 0;
      uint1 flushed = 0;
    } csr;
  } fpu;

  //fpu-instructions.cpp
  auto instructionBC1() -> void;
  auto instructionCFC1() -> void;
  auto instructionCTC1() -> void;
  auto instructionDMFC1() -> void;
  auto instructionDMTC1() -> void;
  auto instructionFABS() -> void;
  auto instructionFADD() -> void;
  auto instructionFCEILL() -> void;
  auto instructionFCEILW() -> void;
  auto instructionFCEQ() -> void;
  auto instructionFCF() -> void;
  auto instructionFCOLE() -> void;
  auto instructionFCOLT() -> void;
  auto instructionFCUEQ() -> void;
  auto instructionFCULE() -> void;
  auto instructionFCULT() -> void;
  auto instructionFCUN() -> void;
  auto instructionFCVTS() -> void;
  auto instructionFCVTD() -> void;
  auto instructionFCVTL() -> void;
  auto instructionFCVTW() -> void;
  auto instructionFDIV() -> void;
  auto instructionFFLOORL() -> void;
  auto instructionFFLOORW() -> void;
  auto instructionFMOV() -> void;
  auto instructionFMUL() -> void;
  auto instructionFNEG() -> void;
  auto instructionFROUNDL() -> void;
  auto instructionFROUNDW() -> void;
  auto instructionFSQRT() -> void;
  auto instructionFSUB() -> void;
  auto instructionFTRUNCL() -> void;
  auto instructionFTRUNCW() -> void;
  auto instructionLDC1() -> void;
  auto instructionLWC1() -> void;
  auto instructionMFC1() -> void;
  auto instructionMTC1() -> void;
  auto instructionSDC1() -> void;
  auto instructionSWC1() -> void;
//};
