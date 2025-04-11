struct Accuracy {
  //enable all accuracy flags
  static constexpr bool Reference = 0;

  struct CPU {
    static constexpr bool Interpreter = 0 | Reference | !recompiler::generic::supported;
    static constexpr bool Recompiler = !Interpreter;

    //exceptions when the CPU accesses unaligned memory addresses
    static constexpr bool AddressErrors = 1 | Reference;
  };

  struct RSP {
    static constexpr bool Interpreter = 0 | Reference | !recompiler::generic::supported;
    static constexpr bool Recompiler = !Interpreter;

    //VU instructions
    static constexpr bool SISD = 0 | Reference | !ARCHITECTURE_SUPPORTS_SSE4_1;
    static constexpr bool SIMD = !SISD;
  };

  struct RDRAM {
    static constexpr bool Broadcasting = 0;
  };

  struct PIF {
    // Emulate a region-locked console
    static constexpr bool RegionLock = false;
    // Emulate the PIF's checksum security check
    static constexpr bool IPL2Checksum = true;
  };

  struct Cartridge {
    // Don't clear the base address of an inserted GameShark when resetting
    static constexpr bool GameSharkReset = 0 | Reference;
  };
};
