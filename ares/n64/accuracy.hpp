struct Accuracy {
  //enable all accuracy flags
  static constexpr bool Reference = 0;

  struct CPU {
    static constexpr bool Interpreter = 0 | Reference;
    static constexpr bool Recompiler = !Interpreter;

    //exceptions when the CPU accesses unaligned memory addresses
    static constexpr bool AddressErrors = 1 | Reference;
  };

  struct RSP {
    static constexpr bool Interpreter = 0 | Reference;
    static constexpr bool Recompiler = !Interpreter;

    //VU instructions
    static constexpr bool SISD = 0 | Reference | !Architecture::amd64 | !Architecture::sse41;
    static constexpr bool SIMD = !SISD;
  };

  struct RDRAM {
    static constexpr bool Broadcasting = 0;
  };
};
