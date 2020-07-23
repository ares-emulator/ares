//{
  //Geometry Transformation Engine
  struct GTE {
    //color
    struct c32 {
      u8 r, g, b, t;
    };

    //screen
    struct s16 {
      i16 x, y;
      u16 z;
    };

    //16-bit vector
    struct v16 { union {
      struct { i16 x, y, z; };
      struct { i16 r, g, b; };
    };};

    //32-bit vector
    struct v32 { union {
      struct { i32 x, y, z; };
      struct { i32 r, g, b; };
    };};

    //64-bit vector
    struct v64 {
      i64 x, y, z;
    };

    //16-bit vector with temporary
    struct v16t : v16 {
      i16 t;
    };

    //32-bit vector with temporary
    struct v32t : v32 {
      i32 t;
    };

    //16-bit matrix
    struct m16 {
      v16 a, b, c;
    };

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
    auto setMacAndIr(const v64& vector) -> void;
    auto setOtz(i64 value) -> void;

    auto matrixMultiply(const m16&, const v16&, const v32& = {0, 0, 0}) -> v64;
    auto vectorMultiply(const v16&, const v16&, const v16& = {0, 0, 0}) -> v64;
    auto divide(u32 lhs, u32 rhs) -> u32;
    auto pushScreenX(i32 sx) -> void;
    auto pushScreenY(i32 sy) -> void;
    auto pushScreenZ(i32 sz) -> void;
    auto pushColor(u32 r, u32 g, u32 b) -> void;
    auto pushColor() -> void;

    auto avsz3() -> void;
    auto avsz4() -> void;
    auto cc() -> void;
    auto cdp() -> void;
    auto dcpl() -> void;
    auto dpc(const v16&) -> void;
    auto dpcs() -> void;
    auto dpct() -> void;
    auto gpf() -> void;
    auto gpl() -> void;
    auto intpl() -> void;
    auto mvmva() -> void;
    template<uint> auto nc(const v16&) -> void;
    auto nccs() -> void;
    auto ncct() -> void;
    auto ncds() -> void;
    auto ncdt() -> void;
    auto nclip() -> void;
    auto ncs() -> void;
    auto nct() -> void;
    auto op() -> void;
    auto rtp(v16, bool last) -> void;
    auto rtps() -> void;
    auto rtpt() -> void;
    auto sqr() -> void;

    m16  v;                //VX, VY, VZ
    c32  rgbc;
    u16  otz;
    v16t ir;
    s16  screen[4];        //SX,  SY,  SZ (screen[3].{x,y} do not exist)
    u32  rgb[4];           //RGB3 is reserved
    v32t mac;
    u32  irgb, orgb;
    u32  lzcs, lzcr;
    m16  rotation;         //RT1, RT2, RT3
    v32  translation;      //TRX, TRY, TRZ
    m16  light;            //L1,  L2,  L3
    v32  backgroundColor;  //RBK, GBK, BBK
    m16  color;            //LR,  LG,  LB
    v32  farColor;         //RFC, GFC, BFC
    i32  ofx, ofy;
    u16  h;
    i16  dqa;
    i32  dqb;
    i16  zsf3, zsf4;
    struct Flag {
      u32 value;
      BitField<32, 12> ir0_saturated  {&value};
      BitField<32, 13> sy2_saturated  {&value};
      BitField<32, 14> sx2_saturated  {&value};
      BitField<32, 15> mac0_underflow {&value};
      BitField<32, 16> mac0_overflow  {&value};
      BitField<32, 17> divide_overflow{&value};
      BitField<32, 18> sz3_saturated  {&value};
      BitField<32, 18> otz_saturated  {&value};
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
      u8 tv;
      u8 mv;
      u8 mm;
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
  auto instructionCC(bool lm, u8 sf) -> void;
  auto instructionCDP(bool lm, u8 sf) -> void;
  auto instructionDCPL(bool lm, u8 sf) -> void;
  auto instructionDPCS(bool lm, u8 sf) -> void;
  auto instructionDPCT(bool lm, u8 sf) -> void;
  auto instructionGPF(bool lm, u8 sf) -> void;
  auto instructionGPL(bool lm, u8 sf) -> void;
  auto instructionINTPL(bool lm, u8 sf) -> void;
  auto instructionMVMVA(bool lm, u8 tv, u8 mv, u8 mm, u8 sf) -> void;
  auto instructionNCCS(bool lm, u8 sf) -> void;
  auto instructionNCCT(bool lm, u8 sf) -> void;
  auto instructionNCDS(bool lm, u8 sf) -> void;
  auto instructionNCDT(bool lm, u8 sf) -> void;
  auto instructionNCLIP() -> void;
  auto instructionNCS(bool lm, u8 sf) -> void;
  auto instructionNCT(bool lm, u8 sf) -> void;
  auto instructionOP(bool lm, u8 sf) -> void;
  auto instructionRTPS(bool lm, u8 sf) -> void;
  auto instructionRTPT(bool lm, u8 sf) -> void;
  auto instructionSQR(bool lm, u8 sf) -> void;
//};
