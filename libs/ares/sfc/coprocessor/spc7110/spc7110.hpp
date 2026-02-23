struct Decompressor;

struct SPC7110 : Thread {
  SPC7110();
  ~SPC7110();

  auto main() -> void;
  auto unload() -> void;
  auto power() -> void;

  auto addClocks(u32 clocks) -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  auto mcuromRead(n24 address, n8 data) -> n8;
  auto mcuromWrite(n24 address, n8 data) -> void;

  auto mcuramRead(n24 address, n8 data) -> n8;
  auto mcuramWrite(n24 address, n8 data) -> void;

  auto serialize(serializer&) -> void;

  //dcu.cpp
  auto dcuLoadAddress() -> void;
  auto dcuBeginTransfer() -> void;
  auto dcuRead() -> n8;

  auto deinterleave1bpp(u32 length) -> void;
  auto deinterleave2bpp(u32 length) -> void;
  auto deinterleave4bpp(u32 length) -> void;

  //data.cpp
  auto dataromRead(n24 address) -> n8;

  auto dataOffset() -> n24;
  auto dataAdjust() -> n16;
  auto dataStride() -> n16;

  auto setDataOffset(n24 address) -> void;
  auto setDataAdjust(n16 address) -> void;

  auto dataPortRead() -> void;

  auto dataPortIncrement4810() -> void;
  auto dataPortIncrement4814() -> void;
  auto dataPortIncrement4815() -> void;
  auto dataPortIncrement481a() -> void;

  //alu.cpp
  auto aluMultiply() -> void;
  auto aluDivide() -> void;

  ReadableMemory prom;  //program ROM
  ReadableMemory drom;  //data ROM
  WritableMemory ram;

private:
  //decompression unit
  n8  r4801;  //compression table B0
  n8  r4802;  //compression table B1
  n7  r4803;  //compression table B2
  n8  r4804;  //compression table index
  n8  r4805;  //adjust length B0
  n8  r4806;  //adjust length B1
  n8  r4807;  //stride length
  n8  r4809;  //compression counter B0
  n8  r480a;  //compression counter B1
  n8  r480b;  //decompression settings
  n8  r480c;  //decompression status

  n1  dcuPending;
  n2  dcuMode;
  n23 dcuAddress;
  n32 dcuOffset;
  n8  dcuTile[32];
  Decompressor* decompressor = nullptr;

  //data port unit
  n8  r4810;  //data port read + seek
  n8  r4811;  //data offset B0
  n8  r4812;  //data offset B1
  n7  r4813;  //data offset B2
  n8  r4814;  //data adjust B0
  n8  r4815;  //data adjust B1
  n8  r4816;  //data stride B0
  n8  r4817;  //data stride B1
  n8  r4818;  //data port settings
  n8  r481a;  //data port seek

  //arithmetic logic unit
  n8  r4820;  //16-bit multiplicand B0, 32-bit dividend B0
  n8  r4821;  //16-bit multiplicand B1, 32-bit dividend B1
  n8  r4822;  //32-bit dividend B2
  n8  r4823;  //32-bit dividend B3
  n8  r4824;  //16-bit multiplier B0
  n8  r4825;  //16-bit multiplier B1
  n8  r4826;  //16-bit divisor B0
  n8  r4827;  //16-bit divisor B1
  n8  r4828;  //32-bit product B0, 32-bit quotient B0
  n8  r4829;  //32-bit product B1, 32-bit quotient B1
  n8  r482a;  //32-bit product B2, 32-bit quotient B2
  n8  r482b;  //32-bit product B3, 32-bit quotient B3
  n8  r482c;  //16-bit remainder B0
  n8  r482d;  //16-bit remainder B1
  n8  r482e;  //math settings
  n8  r482f;  //math status

  n1  mulPending;
  n1  divPending;

  //memory control unit
  n8  r4830;  //bank 0 mapping + SRAM write enable
  n8  r4831;  //bank 1 mapping
  n8  r4832;  //bank 2 mapping
  n8  r4833;  //bank 3 mapping
  n8  r4834;  //bank mapping settings
};

extern SPC7110 spc7110;
