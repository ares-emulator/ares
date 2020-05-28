//{
  union r128 {
    struct { uint128_t u128; };
    struct {   __m128i v128; };
    operator __m128i() const { return v128; }
    auto operator=(__m128i value) { v128 = value; }
    auto byte(uint index) -> u8& { return ((u8*)&u128)[15 - index]; }
    auto element(uint index) -> u16& { return ((u16*)&u128)[7 - index]; }
  };

  //vu-instructions.cpp
  auto vte() -> r128;

  struct COP2 {
    r128 r[32];
    r128 acch, accm, accl;
    r128 vcoh, vcol;  //16-bit little endian
    r128 vcch, vccl;  //16-bit little endian
    r128 vce;         // 8-bit little endian
     i16 divin;
     i16 divout;
    bool divdp;
  } vu;

  u16 reciprocals[512];
  u16 inverseSquareRoots[512];

  static constexpr r128 zero{0};
  static constexpr r128 invert{u128(0) - 1};

  //vu-instructions.cpp
  auto instructionCFC2() -> void;
  auto instructionCTC2() -> void;
  auto instructionLBV() -> void;
  auto instructionLDV() -> void;
  auto instructionLFV() -> void;
  auto instructionLHV() -> void;
  auto instructionLLV() -> void;
  auto instructionLPV() -> void;
  auto instructionLQV() -> void;
  auto instructionLRV() -> void;
  auto instructionLSV() -> void;
  auto instructionLTV() -> void;
  auto instructionLUV() -> void;
  auto instructionLWC2() -> void;
  auto instructionLWV() -> void;
  auto instructionMFC2() -> void;
  auto instructionMTC2() -> void;
  auto instructionSBV() -> void;
  auto instructionSDV() -> void;
  auto instructionSFV() -> void;
  auto instructionSHV() -> void;
  auto instructionSLV() -> void;
  auto instructionSPV() -> void;
  auto instructionSQV() -> void;
  auto instructionSRV() -> void;
  auto instructionSSV() -> void;
  auto instructionSTV() -> void;
  auto instructionSUV() -> void;
  auto instructionSWC2() -> void;
  auto instructionSWV() -> void;
  auto instructionVABS() -> void;
  auto instructionVADD() -> void;
  auto instructionVADDC() -> void;
  auto instructionVAND() -> void;
  auto instructionVCL() -> void;
  auto instructionVCH() -> void;
  auto instructionVCR() -> void;
  auto instructionVEQ() -> void;
  auto instructionVGE() -> void;
  auto instructionVLT() -> void;
  auto instructionVMACF(bool) -> void;
  auto instructionVMACQ() -> void;
  auto instructionVMADH() -> void;
  auto instructionVMADL() -> void;
  auto instructionVMADM() -> void;
  auto instructionVMADN() -> void;
  auto instructionVMOV() -> void;
  auto instructionVMRG() -> void;
  auto instructionVMUDH() -> void;
  auto instructionVMUDL() -> void;
  auto instructionVMUDM() -> void;
  auto instructionVMUDN() -> void;
  auto instructionVMULF(bool) -> void;
  auto instructionVMULQ() -> void;
  auto instructionVNAND() -> void;
  auto instructionVNE() -> void;
  auto instructionVNOP() -> void;
  auto instructionVNOR() -> void;
  auto instructionVNXOR() -> void;
  auto instructionVOR() -> void;
  auto instructionVRCP(bool) -> void;
  auto instructionVRCPH() -> void;
  auto instructionVRND(bool) -> void;
  auto instructionVRSQ(bool) -> void;
  auto instructionVRSQH() -> void;
  auto instructionVSAR() -> void;
  auto instructionVSUB() -> void;
  auto instructionVSUBC() -> void;
  auto instructionVXOR() -> void;
//};
