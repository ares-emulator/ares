struct Decompressor {
  struct IM {  //input manager
    IM(SDD1::Decompressor& self) : self(self) {}
    auto init(n32 offset) -> void;
    auto getCodeWord(n8 codeLength) -> n8;
    auto serialize(serializer&) -> void;

  private:
    Decompressor& self;
    n32 offset;
    n32 bitCount;
  };

  struct GCD {  //golomb-code decoder
    GCD(SDD1::Decompressor& self) : self(self) {}
    auto getRunCount(n8 codeNumber, n8& mpsCount, bool& lpsIndex) -> void;
    auto serialize(serializer&) -> void;

  private:
    Decompressor& self;
    static const n8 runCount[256];
  };

  struct BG {  //bits generator
    BG(SDD1::Decompressor& self, n8 codeNumber) : self(self), codeNumber(codeNumber) {}
    auto init() -> void;
    auto getBit(bool& endOfRun) -> n8;
    auto serialize(serializer&) -> void;

  private:
    Decompressor& self;
    const n8 codeNumber;
    n8 mpsCount;
    bool lpsIndex;
  };

  struct PEM {  //probability estimation module
    PEM(SDD1::Decompressor& self) : self(self) {}
    auto init() -> void;
    auto getBit(n8 context) -> n8;
    auto serialize(serializer&) -> void;

  private:
    Decompressor& self;
    struct State {
      n8 codeNumber;
      n8 nextIfMps;
      n8 nextIfLps;
    };
    static const State evolutionTable[33];
    struct ContextInfo {
      n8 status;
      n8 mps;
    } contextInfo[32];
  };

  struct CM {  //context model
    CM(SDD1::Decompressor& self) : self(self) {}
    auto init(n32 offset) -> void;
    auto getBit() -> n8;
    auto serialize(serializer&) -> void;

  private:
    Decompressor& self;
    n8  bitplanesInfo;
    n8  contextBitsInfo;
    n8  bitNumber;
    n8  currentBitplane;
    n16 previousBitplaneBits[8];
  };

  struct OL {  //output logic
    OL(SDD1::Decompressor& self) : self(self) {}
    auto init(n32 offset) -> void;
    auto decompress() -> n8;
    auto serialize(serializer&) -> void;

  private:
    Decompressor& self;
    n8 bitplanesInfo;
    n8 r0, r1, r2;
  };

  Decompressor();
  auto init(n32 offset) -> void;
  auto read() -> n8;
  auto serialize(serializer&) -> void;

  IM  im;
  GCD gcd;
  BG  bg0, bg1, bg2, bg3, bg4, bg5, bg6, bg7;
  PEM pem;
  CM  cm;
  OL  ol;
};
