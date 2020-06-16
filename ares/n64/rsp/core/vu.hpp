//{
  union r128 {
    struct { uint128_t u128; };
    struct {   __m128i v128; };

    operator __m128i() const { return v128; }
    auto operator=(__m128i value) { v128 = value; }

    auto byte(uint index) -> uint8_t& { return ((uint8_t*)&u128)[15 - index]; }
    auto byte(uint index) const -> uint8_t { return ((uint8_t*)&u128)[15 - index]; }

    auto element(uint index) -> uint16_t& { return ((uint16_t*)&u128)[7 - index]; }
    auto element(uint index) const -> uint16_t { return ((uint16_t*)&u128)[7 - index]; }

    auto u8(uint index) -> uint8_t& { return ((uint8_t*)&u128)[15 - index]; }
    auto u8(uint index) const -> uint8_t { return ((uint8_t*)&u128)[15 - index]; }

    auto i16(uint index) -> int16_t& { return ((int16_t*)&u128)[7 - index]; }
    auto i16(uint index) const -> int16_t { return ((int16_t*)&u128)[7 - index]; }

    auto u16(uint index) -> uint16_t& { return ((uint16_t*)&u128)[7 - index]; }
    auto u16(uint index) const -> uint16_t { return ((uint16_t*)&u128)[7 - index]; }

    //VCx registers
    auto get(uint index) const -> bool { return u16(index) != 0; }
    auto set(uint index, bool value) -> bool { return u16(index) = 0 - value, value; }

    //vu-registers.cpp
    auto operator()(uint index) const -> r128;
  };
  using cr128 = const r128;

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

  //vu-registers.cpp
  auto accumulatorGet(uint index) const -> u64;
  auto accumulatorSet(uint index, u64 value) -> void;
  auto accumulatorSaturate(uint index, bool slice, u16 negative, u16 positive) const -> u16;

  u16 reciprocals[512];
  u16 inverseSquareRoots[512];

  static constexpr r128 zero{0};
  static constexpr r128 invert{u128(0) - 1};

  //vu-instructions.cpp
  auto instructionCFC2(r32& rt, u8 rd) -> void;
  auto instructionCTC2(cr32& rt, u8 rd) -> void;
  auto instructionLBV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLDV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLFV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLHV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLLV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLPV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLQV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLRV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLSV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLTV(u8 vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLUV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionLWV(r128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionMFC2(r32& rt, cr128& vs, u8 e) -> void;
  auto instructionMTC2(cr32& rt, r128& vs, u8 e) -> void;
  auto instructionSBV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSDV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSFV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSHV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSLV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSPV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSQV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSRV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSSV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSTV(u8 vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSUV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionSWV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void;
  auto instructionVABS(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVADD(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVADDC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVAND(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVCH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVCL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVCR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVEQ(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVGE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVLT(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool U>
  auto instructionVMACF(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMACQ(r128& vd) -> void;
  auto instructionVMADH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMADL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMADM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMADN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMOV(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVMRG(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool U>
  auto instructionVMULF(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMULQ(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNAND(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNOP() -> void;
  auto instructionVNOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNXOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool L>
  auto instructionVRCP(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVRCPH(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  template<bool D>
  auto instructionVRND(r128& vd, u8 vs, cr128& vt, u8 e) -> void;
  template<bool L>
  auto instructionVRSQ(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVRSQH(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVSAR(r128& vd, cr128& vs, u8 e) -> void;
  auto instructionVSUB(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVSUBC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVXOR(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
//};
