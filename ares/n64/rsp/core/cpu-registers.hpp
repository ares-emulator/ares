//{
  struct Core {
    enum Register : uint {
      R0, AT, V0, V1, A0, A1, A2, A3,
      T0, T1, T2, T3, T4, T5, T6, T7,
      S0, S1, S2, S3, S4, S5, S6, S7,
      T8, T9, K0, K1, GP, SP, S8, RA,
    };

    r64 r[32];
    r64 lo;
    r64 hi;
    u64 pc;

    //internal
    maybe<u64> ip;
  } core;
//};
