//struct CPU {
  struct GTE {
    //color
    struct c32 {
      u8 r, g, b, t;
    };

    //screen point
    struct p16 {
      s16 x, y;
      u16 z;
    };

    //16-bit vector
    struct v16 { union {
      struct { s16 x, y, z; };
      struct { s16 r, g, b; };
    };};

    //32-bit vector
    struct v32 { union {
      struct { s32 x, y, z; };
      struct { s32 r, g, b; };
    };};

    //64-bit vector
    struct v64 {
      s64 x, y, z;
    };

    //16-bit vector with temporary
    struct v16t : v16 {
      s16 t;
    };

    //32-bit vector with temporary
    struct v32t : v32 {
      s32 t;
    };

    //16-bit matrix
    struct m16 {
      v16 a, b, c;
    };

    auto power() -> void;
    auto serialize(serializer&) -> void;
    auto commandCycles(u8 command) const -> u32;
    auto constructTable() -> void;

    auto countLeadingZeroes16(u16) -> u32;
    auto countLeadingZeroes32(u32) -> u32;

    auto getDataRegister(u32) -> u32;
    auto setDataRegister(u32, u32) -> void;

    auto getControlRegister(u32) -> u32;
    auto setControlRegister(u32, u32) -> void;

    template<u32> auto checkMac(s64 value) -> s64;
    template<u32> auto extend(s64 mac) -> s64;
    template<u32> auto saturateIr(s32 value, bool lm = 0) -> s32;
    template<u32> auto saturateColor(s32 value) -> u8;

    template<u32> auto setMac(s64 value) -> s64;
    template<u32> auto setIr(s32 value, bool lm = 0) -> void;
    template<u32> auto setMacAndIr(s64 value, bool lm = 0) -> void;
    auto setMacAndIr(const v64& vector) -> void;
    auto setOtz(s64 value) -> void;

    auto matrixMultiply(const m16&, const v16&, const v32& = {0, 0, 0}) -> v64;
    auto vectorMultiply(const v16&, const v16&, const v16& = {0, 0, 0}) -> v64;
    auto vectorMultiply(const v16&, s16) -> v64;
    auto divide(u32 lhs, u32 rhs) -> u32;
    auto pushScreenX(s32 sx) -> void;
    auto pushScreenY(s32 sy) -> void;
    auto pushScreenZ(s32 sz) -> void;
    auto pushColor(s32 r, s32 g, s32 b) -> void;
    auto pushColor() -> void;

    auto prologue() -> void;
    auto prologue(bool lm, u8 sf) -> void;
    auto epilogue() -> void;

    auto AVSZ3() -> void;
    auto AVSZ4() -> void;
    auto CC(bool lm, u8 sf) -> void;
    auto CDP(bool lm, u8 sf) -> void;
    auto DCPL(bool lm, u8 sf) -> void;
    auto DPC(const v16&) -> void;
    auto DPCS(bool lm, u8 sf) -> void;
    auto DPCT(bool lm, u8 sf) -> void;
    auto GPF(bool lm, u8 sf) -> void;
    auto GPL(bool lm, u8 sf) -> void;
    auto INTPL(bool lm, u8 sf) -> void;
    auto MVMVA(bool lm, u8 tv, u8 mv, u8 mm, u8 sf) -> void;
    auto MVMVA_(bool lm, u8 MmMvTv, u8 sf) -> void;
    template<u32> auto NC(const v16&) -> void;
    auto NCCS(bool lm, u8 sf) -> void;
    auto NCCT(bool lm, u8 sf) -> void;
    auto NCDS(bool lm, u8 sf) -> void;
    auto NCDT(bool lm, u8 sf) -> void;
    auto NCLIP() -> void;
    auto NCS(bool lm, u8 sf) -> void;
    auto NCT(bool lm, u8 sf) -> void;
    auto OP(bool lm, u8 sf) -> void;
    auto RTP(v16, bool last) -> void;
    auto RTPS(bool lm, u8 sf) -> void;
    auto RTPT(bool lm, u8 sf) -> void;
    auto SQR(bool lm, u8 sf) -> void;

    m16  v;                //VX, VY, VZ
    c32  rgbc;
    u16  otz;
    v16t ir;
    p16  screen[4];        //SX,  SY,  SZ (screen[3].{x,y} do not exist)
    u32  rgb[4];           //RGB3 is reserved
    v32t mac;
    u32  lzcs, lzcr;
    m16  rotation;         //RT1, RT2, RT3
    v32  translation;      //TRX, TRY, TRZ
    m16  light;            //L1,  L2,  L3
    v32  backgroundColor;  //RBK, GBK, BBK
    m16  color;            //LR,  LG,  LB
    v32  farColor;         //RFC, GFC, BFC
    s32  ofx, ofy;
    u16  h;
    s16  dqa;
    s32  dqb;
    s16  zsf3, zsf4;
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
    u8   tv;
    u8   mv;
    u8   mm;
    u8   sf;

  //unserialized:
    u8 unsignedNewtonRaphsonTable[257];
  } gte;
//};
