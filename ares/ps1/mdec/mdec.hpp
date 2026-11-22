//Macroblock Decoder

struct MDEC : Thread, Memory::Interface {
  Node::Object node;

  //mdec.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto power(bool reset) -> void;
  auto main() -> void;
  auto step(u32 cycles) -> void;

  //decoder.cpp
  auto decodeMacroblock() -> bool;
  auto writeBlock(u32 bx = 0, u32 by = 0) -> void;
  auto decodeBlock(s16 block[64]) -> bool;
  auto prefetchInput() -> void;
  template<u32 Pass> auto decodeIDCT(s16 source[64], s16 target[64]) -> void;
  auto convertY(u32 output[64], s16 luma[64]) -> void;
  auto convertYUV(u32 output[256], s16 luma[64], u32 bx, u32 by) -> void;

  //io.cpp
  auto notifyDMA() -> void;
  auto canReadDMA() -> bool;
  auto canWriteDMA() -> bool;
  auto readDMA() -> u32;
  auto dmaAddress(u32 address) const -> u32;
  auto writeDMA(u32 data) -> void;

  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto processInput() -> void;

  auto readInputFifo() -> maybe<u16>;
  auto writeOutputFifo(u32 data) -> void;
  auto refillOutputFifo() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct FIFO {
    queue<u16[64]> input;
    queue<u16[64]> inputBuffer;
    queue<u32[32]> output;
    queue<u32[48]> pendingOutput;
  } fifo;

  struct Status {
    n32 remaining;
    n1  noOperation;
    n16 noOperationValue;
    n3  currentBlock;
    n1  outputMaskBit;
    n1  outputSigned;
    n2  outputDepth;
    n1  outputRequest;
    n1  inputRequest;
  } status;

  u32 output[256];

  enum Mode : u32 { Idle, DecodeMacroblock, SetQuantTable, SetScaleTable, DrainOutput };

  struct IO {
    Mode mode;
    n1   resetPending;
    n32  offset;
    n32  outputWord;
    n32  remaining;
    n3   decodeBlock;
  } io;

  struct Prefetch {
    s16 coefficient[64];
    n3 block;
    n7 index;
    n6 qfactor;
    n1 started;
    n1 ready;
  } prefetch;

  struct Block {
    u8  luma[64];
    u8  chroma[64];
    s16 scale[64];
    s16 cr[64];
    s16 cb[64];
    s16 y0[64];
    s16 y1[64];
    s16 y2[64];
    s16 y3[64];
  } block;

  //tables.cpp
  static const u8 zigzag[64];
  static const u8 zagzig[64];
};

extern MDEC mdec;
