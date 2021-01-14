struct Accuracy {
  //enable all accuracy flags
  static constexpr bool Reference = 0;

  struct CPU {
    static constexpr bool Interpreter = 0 | Reference;
    static constexpr bool Recompiler = !Interpreter;

    //exceptions when the CPU accesses unaligned memory addresses
    static constexpr bool AddressErrors = 0 | Reference & !Recompiler;

    //exceptions when the CPU accesses unmapped memory addresses
    static constexpr bool BusErrors = 0 | Reference & !Recompiler;

    //breakpoints are expensive and not used by any commercial games
    static constexpr bool Breakpoints = 0 | Reference & !Recompiler;
  };

  struct GPU {
    //performs GPU primitive rendering on a separate thread
    static constexpr bool Threaded = 1 & !Reference;
  };
};
