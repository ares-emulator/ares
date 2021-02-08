//Macroblock Decoder

struct MDEC : Thread, Memory::Interface {
  Node::Object node;

  //mdec.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto power(bool reset) -> void;

  //decoder.cpp
  auto decodeMacroblocks() -> void;
  auto decodeBlock(s16 block[64], u8 table[64]) -> bool;
  template<u32 Pass> auto decodeIDCT(s16 source[64], s16 target[64]) -> void;
  auto convertY(u32 output[64], s16 luma[64]) -> void;
  auto convertYUV(u32 output[256], s16 luma[64], u32 bx, u32 by) -> void;

  //io.cpp
  auto readDMA() -> u32;
  auto writeDMA(u32 data) -> void;

  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct FIFO {
    queue<u16[65536]> input;
    queue<u32[65536]> output;
  } fifo;

  struct Status {
    n16 remaining;
    n3  currentBlock;
    n1  outputMaskBit;
    n1  outputSigned;
    n2  outputDepth;
    n1  outputRequest;
    n1  inputRequest;
    n1  commandBusy;
    n1  inputFull;
    n1  outputEmpty;
  } status;

  enum Mode : u32 { Idle, DecodeMacroblock, SetQuantTable, SetScaleTable };

  struct IO {
    Mode mode;
    n32  offset;
  } io;

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
