//{
  //Geometry Transformation Engine
  struct GTE {
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
      struct Saturated {
        bool ir0, ir1, ir2, ir3;
        bool sx2, sy2, sz1_otz;
        bool r, g, b;
      } saturated;
      struct Underflow {
        bool mac0, mac1, mac2, mac3;
      } underflow;
      struct Overflow {
        bool mac0, mac1, mac2, mac3;
        bool division;
      } overflow;
      bool error;
    } flag;
  } gte;

  //gte-instructions.cpp
  auto instructionCFC2(u32& rt, u8 rd) -> void;
  auto instructionCTC2(cu32& rt, u8 rd) -> void;
  auto instructionMFC2(u32& rt, u8 rd) -> void;
  auto instructionMTC2(cu32& rt, u8 rd) -> void;

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
  auto instructionNCDS() -> void;
  auto instructionNCDT() -> void;
  auto instructionNCLIP() -> void;
  auto instructionNCS() -> void;
  auto instructionNCT() -> void;
  auto instructionOP() -> void;
  auto instructionRTPS() -> void;
  auto instructionRTPT() -> void;
  auto instructionSQR() -> void;
//};
