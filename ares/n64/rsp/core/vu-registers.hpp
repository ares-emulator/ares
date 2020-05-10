//{
  struct COP2 {
    v128 r[32];
    v128 acch, accm, accl;
    v128 vcoh, vcol;  //16-bit little endian
    v128 vcch, vccl;  //16-bit little endian
    v128 vce;         // 8-bit little endian
     i32 divin;
     i32 divout;
    bool divdp;
  } cop2;

  u16 reciprocals[512];
  u16 inverseSquareRoots[512];

  static constexpr v128 zero{0};
  static constexpr v128 invert{u128(0) - 1};
//};
