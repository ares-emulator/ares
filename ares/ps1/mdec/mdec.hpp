//Macroblock Decoder

struct MDEC : Thread, Memory::Interface {
  Node::Object node;
  enum BlockProgress : u32;

  //mdec.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto power(bool reset) -> void;
  auto main() -> void;
  auto step(u32 cycles) -> void;

  //decoder.cpp
  auto advanceDecode() -> void;
  auto advanceBlock(s16 block[64], u8 table[64]) -> BlockProgress;
  auto fillOutputFifo() -> void;
  auto outputWord(u32 outputBlock, u32 word) const -> u32;
  auto output15(u32 color) const -> u16;
  auto referenceDecodeClocks() const -> u32;
  template<u32 Pass> auto decodeIDCT(s16 source[64], s16 target[64]) -> void;
  auto convertY(u32 output[64], s16 luma[64]) -> void;
  auto convertYUV(u32 output[256], s16 luma[64], u32 bx, u32 by) -> void;

  //io.cpp
  auto canReadDMA() -> bool;
  auto canWriteDMA() -> bool;
  auto dmaOffset() const -> s32;
  auto readDMA() -> u32;
  auto writeDMA(u32 data) -> void;

  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  auto readInputFifo() -> maybe<u16>;
  auto writeOutputFifo(u32 data) -> void;
  auto outputWordsPerBlock() const -> u32;
  auto outputPixel(u32 index) const -> u32;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct FIFO {
    queue<u16[64]> input;
    //Compatibility staging capacity until the hardware probe establishes the physical FIFO size.
    queue<u32[32]> output;
  } fifo;

  struct ParameterState {
    auto read() const -> u16 {
      if(value.bit(31)) return value.bit(0,15);
      return (value / 2) - 1;
    }

    auto setDirect(u16 direct) -> void {
      value = direct;
      value.bit(31) = 1;
    }

    auto setHalfwords(u32 halfwords) -> void { value = halfwords; }
    auto halfwords() const -> u32 { return value.bit(31) ? 0 : (u32)value; }
    auto empty() const -> bool { return halfwords() == 0; }
    auto consume(u32 count) -> void { value -= count; }
    operator u32() const { return halfwords(); }

    n32 value;
  };

  struct Status {
    ParameterState remaining;
    n3  currentBlock;
    n1  outputMaskBit;
    n1  outputSigned;
    n2  outputDepth;
    n1  outputRequest;
    n1  inputRequest;
  } status;

  u32 output[256];

  enum Mode : u32 { Idle, DecodeMacroblock, SetQuantTable, SetScaleTable, NoCommand };
  enum DecodePhase : u32 {
    DecodeIdle, DecodeCr, DecodeCb, DecodeY0, DecodeY1, DecodeY2, DecodeY3,
    Convert, Publish, Drain,
  };
  enum BlockPhase : u32 { BlockStart, BlockRLE };
  enum BlockProgress : u32 { BlockWaiting, BlockComplete, BlockAborted };

  //Reference-compatibility timing from DuckStation b0f7c5c1624d. This is not
  //retail-hardware evidence; the timing probe still owns the final phase model.
  static constexpr u32 ReferenceBlockClocks = 448;
  static constexpr u32 ReferenceMacroblockClocks = ReferenceBlockClocks * 6;

  struct IO {
    Mode mode;
    DecodePhase decodePhase;
    BlockPhase blockPhase;
    n32  offset;
    u32  outputOffset;
    u32  outputBlock;
    u32  outputWriteOffset;
    u32  coefficient;
    u32  qfactor;
    u32  phaseClocks;
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
