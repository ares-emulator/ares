struct Accuracy {
  //enable all accuracy flags
  static constexpr bool Reference = 0;

  struct CPU {
    //0 = dynamic recompiler; 1 = interpreter
    static constexpr bool Interpreter = 0 | Reference;

    //exceptions when the CPU accesses unaligned memory addresses
    static constexpr bool AddressErrors = 0 | Reference;
  };

  struct RSP {
    //0 = dynamic recompiler; 1 = interpreter
    static constexpr bool Interpreter = 0 | Reference;

    //VU instructions
    static constexpr bool SISD = 0 | Reference;
    static constexpr bool SIMD = SISD == 0;
  };
};
