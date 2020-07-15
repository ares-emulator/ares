//{
  //Geometry Transformation Engine
  struct GTE {
    //gte.cpp
    auto constructTable() -> void;

    auto clearFlags() -> void;
    auto updateError() -> void;

    auto getDataRegister(uint) -> u32;
    auto setDataRegister(uint, u32) -> void;

    auto getControlRegister(uint) -> u32;
    auto setControlRegister(uint, u32) -> void;

    template<uint> auto check(i64 value) -> i64;
    template<uint> auto extend(i64 mac) -> i64;
    template<uint> auto saturate(i32 value, bool lm = 0) -> i32;
    template<uint> auto saturateRGB(i32 value) -> u32;

    template<uint> auto setMac(i64 value) -> i64;
    template<uint> auto setIr(i32 value, bool lm = 0) -> void;
    template<uint> auto setMacAndIr(i64 value, bool lm = 0) -> void;
    auto setOtz(i64 value) -> void;

    auto divide(u32 lhs, u32 rhs) -> u32;
    auto pushSX(i32 sx) -> void;
    auto pushSY(i32 sy) -> void;
    auto pushSZ(i32 sz) -> void;
    auto pushColor(u32 r, u32 g, u32 b) -> void;
    auto pushColor() -> void;

    auto avsz3() -> void;
    auto avsz4() -> void;
    auto cc() -> void;
    auto cdp() -> void;
    auto dcpl() -> void;
    auto dpcs() -> void;
    auto dpct() -> void;
    auto gpf() -> void;
    auto gpl() -> void;
    auto intpl() -> void;
    auto mvmva() -> void;
    template<uint> auto nc(i32 vx, i32 vy, i32 vz) -> void;
    auto nccs() -> void;
    auto ncct() -> void;
    auto ncds() -> void;
    auto ncdt() -> void;
    auto nclip() -> void;
    auto ncs() -> void;
    auto nct() -> void;
    auto op() -> void;
    auto rtp(i32 vx, i32 vy, i32 vz, bool last) -> void;
    auto rtps() -> void;
    auto rtpt() -> void;
    auto sqr() -> void;

    i16 vx0, vy0, vz0;
    i16 vx1, vy1, vz1;
    i16 vx2, vy2, vz2;
    u32 rgbc;
    u16 otz;
    i16 ir0, ir1, ir2, ir3;
    i16 sx0, sy0;
    i16 sx1, sy1;
    i16 sx2, sy2;
    u16 sz0, sz1, sz2, sz3;
    u32 rgb0, rgb1, rgb2;
    u32 res1;
    i32 mac0, mac1, mac2, mac3;
    u32 irgb, orgb;
    u32 lzcs, lzcr;
    i16 rt11, rt12, rt13;
    i16 rt21, rt22, rt23;
    i16 rt31, rt32, rt33;
    i32 trx, trY, trz;
    i16 l11, l12, l13;
    i16 l21, l22, l23;
    i16 l31, l32, l33;
    i32 rbk, gbk, bbk;
    i16 lr1, lr2, lr3;
    i16 lg1, lg2, lg3;
    i16 lb1, lb2, lb3;
    i32 rfc, gfc, bfc;
    i32 ofx, ofy;
    u16 h;
    i16 dqa;
    i32 dqb;
    i16 zsf3, zsf4;
    struct Flag {
      u32 value;
      BitField<32, 12> ir0_saturated  {&value};
      BitField<32, 13> sy2_saturated  {&value};
      BitField<32, 14> sx2_saturated  {&value};
      BitField<32, 15> mac0_underflow {&value};
      BitField<32, 16> mac0_overflow  {&value};
      BitField<32, 17> divide_overflow{&value};
      BitField<32, 18> sz3_saturated  {&value};
      BitField<32, 19> otz_saturated  {&value};
      BitField<32, 19> b_saturated    {&value};
      BitField<32, 20> g_saturated    {&value};
      BitField<32, 21> r_saturated    {&value};
      BitField<32, 22> ir3_saturated  {&value};
      BitField<32, 23> ir2_saturated  {&value};
      BitField<32, 24> ir1_saturated  {&value};
      BitField<32, 25> mac3_underflow {&value};
      BitField<32, 26> mac2_underflow {&value};
      BitField<32, 27> mac1_underflow {&value};
      BitField<32, 28> mac3_overflow  {&value};
      BitField<32, 29> mac2_overflow  {&value};
      BitField<32, 30> mac1_overflow  {&value};
      BitField<32, 31> error          {&value};
    } flag;
    bool lm;
      u8 sf;

  //unserialized:
    u8 unsignedNewtonRaphsonTable[257];
  } gte;

  //gte.cpp
  auto constructTableGTE() -> void;

  //gte-instructions.cpp
  auto instructionCFC2(u32& rt, u8 rd) -> void;
  auto instructionCTC2(cu32& rt, u8 rd) -> void;
  auto instructionMFC2(u32& rt, u8 rd) -> void;
  auto instructionMTC2(cu32& rt, u8 rd) -> void;
  auto instructionLWC2(u8 rt, cu32& rs, i16 imm) -> void;
  auto instructionSWC2(u8 rt, cu32& rs, i16 imm) -> void;

  auto instructionAVSZ3() -> void;
  auto instructionAVSZ4() -> void;
  auto instructionCC() -> void;
  auto instructionCDP() -> void;
  auto instructionDCPL() -> void;
  auto instructionDPCS() -> void;
  auto instructionDPCT() -> void;
  auto instructionGPF() -> void;
  auto instructionGPL() -> void;
  auto instructionINTPL() -> void;
  auto instructionMVMVA() -> void;
  auto instructionNCCS() -> void;
  auto instructionNCCT() -> void;
  auto instructionNCDS(bool lm, u8 sf) -> void;
  auto instructionNCDT(bool lm, u8 sf) -> void;
  auto instructionNCLIP() -> void;
  auto instructionNCS() -> void;
  auto instructionNCT() -> void;
  auto instructionOP() -> void;
  auto instructionRTPS(bool lm, u8 sf) -> void;
  auto instructionRTPT(bool lm, u8 sf) -> void;
  auto instructionSQR() -> void;
//};
