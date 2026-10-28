#pragma once

struct SmaConfig {
  u8  dataSwap[16];
  u8  fixedSwap[19];
  u32 fixedSource;
  u8  bankSwap[15];
  u32 bankSwapBits;
  u32 bankSwapLength;
  bool fixedAfterBank;
  u8  bankSelect[6];
  u32 bankOffset[64];
  u32 bankRegister;
  u32 randomPort[2];
  u32 fixBankType;
};

namespace SmaTables {

static constexpr SmaConfig kof99 = {
  .dataSwap = {13, 7, 3, 0, 9, 4, 5, 6, 1,12, 8,14,10,11, 2,15},
  .fixedSwap = {18,11, 6,14,17,16, 5, 8,10,12, 0, 4, 3, 2, 7, 9,15,13, 1},
  .fixedSource = 0x700000,
  .bankSwap = { 6, 2, 4, 9, 8, 3, 1, 7, 0, 5},
  .bankSwapBits = 10,
  .bankSwapLength = 0x600000,
  .fixedAfterBank = true,
  .bankSelect = { 5,12,10, 8, 6,14},
  .bankOffset = {
    0x000000, 0x100000, 0x200000, 0x300000, 0x3cc000, 0x4cc000, 0x3f2000, 0x4f2000,
    0x407800, 0x507800, 0x40d000, 0x50d000, 0x417800, 0x517800, 0x420800, 0x520800,
    0x424800, 0x524800, 0x429000, 0x529000, 0x42e800, 0x52e800, 0x431800, 0x531800,
    0x54d000, 0x551000, 0x567000, 0x592800, 0x588800, 0x581800, 0x599800, 0x594800,
    0x598000,
  },
  .bankRegister = 0x2ffff0,
  .randomPort = {0x2ffff8, 0x2ffffa},
  .fixBankType = 0,
};

static constexpr SmaConfig garou = {
  .dataSwap = {13,12,14,10, 8, 2, 3, 1, 5, 9,11, 4,15, 0, 6, 7},
  .fixedSwap = {18, 4, 5,16,14, 7, 9, 6,13,17,15, 3, 1, 2,12,11, 8,10, 0},
  .fixedSource = 0x710000,
  .bankSwap = { 9, 4, 8, 3,13, 6, 2, 7, 0,12, 1,11,10, 5},
  .bankSwapBits = 14,
  .bankSwapLength = 0x800000,
  .fixedAfterBank = false,
  .bankSelect = {12,14, 6, 7, 9, 5},
  .bankOffset = {
    0x000000, 0x100000, 0x200000, 0x300000, 0x280000, 0x380000, 0x2d0000, 0x3d0000,
    0x2f0000, 0x3f0000, 0x400000, 0x500000, 0x420000, 0x520000, 0x440000, 0x540000,
    0x498000, 0x598000, 0x4a0000, 0x5a0000, 0x4a8000, 0x5a8000, 0x4b0000, 0x5b0000,
    0x4b8000, 0x5b8000, 0x4c0000, 0x5c0000, 0x4c8000, 0x5c8000, 0x4d0000, 0x5d0000,
    0x458000, 0x558000, 0x460000, 0x560000, 0x468000, 0x568000, 0x470000, 0x570000,
    0x478000, 0x578000, 0x480000, 0x580000, 0x488000, 0x588000, 0x490000, 0x590000,
    0x5d0000, 0x5d8000, 0x5e0000, 0x5e8000, 0x5f0000, 0x5f8000, 0x600000,
  },
  .bankRegister = 0x2fffc0,
  .randomPort = {0x2fffcc, 0x2ffff0},
  .fixBankType = 1,
};

static constexpr SmaConfig garouh = {
  .dataSwap = {14, 5, 1,11, 7, 4,10,15, 3,12, 8,13, 0, 2, 9, 6},
  .fixedSwap = {18, 5,16,11, 2, 6, 7,17, 3,12, 8,14, 4, 0, 9, 1,10,15,13},
  .fixedSource = 0x7f8000,
  .bankSwap = {12, 8, 1, 7,11, 3,13,10, 6, 9, 5, 4, 0, 2},
  .bankSwapBits = 14,
  .bankSwapLength = 0x800000,
  .fixedAfterBank = false,
  .bankSelect = {13,11, 2,14, 8, 4},
  .bankOffset = {
    0x000000, 0x100000, 0x200000, 0x300000, 0x280000, 0x380000, 0x2d0000, 0x3d0000,
    0x2c8000, 0x3c8000, 0x400000, 0x500000, 0x420000, 0x520000, 0x440000, 0x540000,
    0x598000, 0x698000, 0x5a0000, 0x6a0000, 0x5a8000, 0x6a8000, 0x5b0000, 0x6b0000,
    0x5b8000, 0x6b8000, 0x5c0000, 0x6c0000, 0x5c8000, 0x6c8000, 0x5d0000, 0x6d0000,
    0x458000, 0x558000, 0x460000, 0x560000, 0x468000, 0x568000, 0x470000, 0x570000,
    0x478000, 0x578000, 0x480000, 0x580000, 0x488000, 0x588000, 0x490000, 0x590000,
    0x5d8000, 0x6d8000, 0x5e0000, 0x6e0000, 0x5e8000, 0x6e8000, 0x6e8000,
  },
  .bankRegister = 0x2fffc0,
  .randomPort = {0x2fffcc, 0x2ffff0},
  .fixBankType = 1,
};

static constexpr SmaConfig mslug3 = {
  .dataSwap = { 4,11,14, 3, 1,13, 0, 7, 2, 8,12,15,10, 9, 5, 6},
  .fixedSwap = {18,15, 2, 1,13, 3, 0, 9, 6,16, 4,11, 5, 7,12,17,14,10, 8},
  .fixedSource = 0x5d0000,
  .bankSwap = { 2,11, 0,14, 6, 4,13, 8, 9, 3,10, 7, 5,12, 1},
  .bankSwapBits = 15,
  .bankSwapLength = 0x800000,
  .fixedAfterBank = false,
  .bankSelect = { 9, 3, 6,15,12,14},
  .bankOffset = {
    0x000000, 0x020000, 0x040000, 0x060000, 0x070000, 0x090000, 0x0b0000, 0x0d0000,
    0x0e0000, 0x0f0000, 0x120000, 0x130000, 0x140000, 0x150000, 0x180000, 0x190000,
    0x1a0000, 0x1b0000, 0x1e0000, 0x1f0000, 0x200000, 0x210000, 0x240000, 0x250000,
    0x260000, 0x270000, 0x2a0000, 0x2b0000, 0x2c0000, 0x2d0000, 0x300000, 0x310000,
    0x320000, 0x330000, 0x360000, 0x370000, 0x380000, 0x390000, 0x3c0000, 0x3d0000,
    0x400000, 0x410000, 0x440000, 0x450000, 0x460000, 0x470000, 0x4a0000, 0x4b0000,
    0x4c0000,
  },
  .bankRegister = 0x2fffe4,
  .randomPort = { 0, 0},
  .fixBankType = 1,
};

static constexpr SmaConfig mslug3a = {
  .dataSwap = { 2,11,12,14, 9, 3, 1, 4,13, 7, 6, 8,10,15, 0, 5},
  .fixedSwap = {18, 1,16,14, 7,17, 5, 8, 4,15, 6, 3, 2, 0,13,10,12, 9,11},
  .fixedSource = 0x5d0000,
  .bankSwap = {12, 0,11, 3, 4,13, 6, 8,14, 7, 5, 2,10, 9, 1},
  .bankSwapBits = 15,
  .bankSwapLength = 0x800000,
  .fixedAfterBank = false,
  .bankSelect = {11,12, 6, 1, 3,15},
  .bankOffset = {
    0x000000, 0x030000, 0x040000, 0x070000, 0x080000, 0x0a0000, 0x0c0000, 0x0e0000,
    0x0f0000, 0x100000, 0x130000, 0x140000, 0x150000, 0x160000, 0x190000, 0x1a0000,
    0x1b0000, 0x1c0000, 0x1f0000, 0x200000, 0x210000, 0x220000, 0x250000, 0x260000,
    0x270000, 0x280000, 0x2b0000, 0x2c0000, 0x2d0000, 0x2e0000, 0x310000, 0x320000,
    0x330000, 0x340000, 0x370000, 0x380000, 0x390000, 0x3a0000, 0x3d0000, 0x3e0000,
    0x400000, 0x410000, 0x440000, 0x450000, 0x460000, 0x470000, 0x4a0000, 0x4b0000,
    0x4c0000,
  },
  .bankRegister = 0x2fffe4,
  .randomPort = { 0, 0},
  .fixBankType = 1,
};

static constexpr SmaConfig kof2000 = {
  .dataSwap = {12, 8,11, 3,15,14, 7, 0,10,13, 6, 5, 9, 2, 1, 4},
  .fixedSwap = {18, 8, 4,15,13, 3,14,16, 2, 6,17, 7,12,10, 0, 5,11, 1, 9},
  .fixedSource = 0x73a000,
  .bankSwap = { 4, 1, 3, 8, 6, 2, 7, 0, 9, 5},
  .bankSwapBits = 10,
  .bankSwapLength = 0x63a000,
  .fixedAfterBank = true,
  .bankSelect = { 5,10, 3, 7,14,15},
  .bankOffset = {
    0x000000, 0x100000, 0x200000, 0x300000, 0x3f7800, 0x4f7800, 0x3ff800, 0x4ff800,
    0x407800, 0x507800, 0x40f800, 0x50f800, 0x416800, 0x516800, 0x41d800, 0x51d800,
    0x424000, 0x524000, 0x523800, 0x623800, 0x526000, 0x626000, 0x528000, 0x628000,
    0x52a000, 0x62a000, 0x52b800, 0x62b800, 0x52d000, 0x62d000, 0x52e800, 0x62e800,
    0x618000, 0x619000, 0x61a000, 0x61a800,
  },
  .bankRegister = 0x2fffec,
  .randomPort = {0x2fffd8, 0x2fffda},
  .fixBankType = 2,
};

}
