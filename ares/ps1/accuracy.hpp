struct Accuracy {
  //enable all accuracy flags
  static constexpr bool Reference = 0;

  struct CPU {
    //exceptions when the CPU accesses unaligned memory addresses
    static constexpr bool AddressErrors = 1 | Reference;

    //exceptions when the CPU accesses unmapped memory addresses
    static constexpr bool BusErrors = 1 | Reference;

    //breakpoints are expensive and not used by any commercial games (but are used by Action Replay, etc)
    static constexpr bool Breakpoints = 1 | Reference;
  };

  struct GPU {
    //performs GPU primitive rendering on a separate thread
    static constexpr bool Threaded = 1;
  };
};
