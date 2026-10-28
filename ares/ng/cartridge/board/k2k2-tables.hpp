#pragma once

struct K2k2Config {
  u32 programBase;
  u32 programBlocks;
  u32 programOrder[16];
  u8  voiceKey;
  u32 fixBank;
};

namespace K2k2Tables {

static constexpr K2k2Config kof2002 = {
  .programBase = 0x100000,
  .programBlocks = 8,
  .programOrder = {0x100000, 0x280000, 0x300000, 0x180000, 0x000000, 0x380000, 0x200000, 0x080000},
  .voiceKey = 0,
  .fixBank = 0,
};

static constexpr K2k2Config matrim = {
  .programBase = 0x100000,
  .programBlocks = 8,
  .programOrder = {0x100000, 0x280000, 0x300000, 0x180000, 0x000000, 0x380000, 0x200000, 0x080000},
  .voiceKey = 1,
  .fixBank = 2,
};

static constexpr K2k2Config samsho5 = {
  .programBase = 0x000000,
  .programBlocks = 16,
  .programOrder = {
    0x000000, 0x080000, 0x700000, 0x680000, 0x500000, 0x180000, 0x200000, 0x480000,
    0x300000, 0x780000, 0x600000, 0x280000, 0x100000, 0x580000, 0x400000, 0x380000,
  },
  .voiceKey = 4,
  .fixBank = 1,
};

static constexpr K2k2Config samsh5sp = {
  .programBase = 0x000000,
  .programBlocks = 16,
  .programOrder = {
    0x000000, 0x080000, 0x500000, 0x480000, 0x600000, 0x580000, 0x700000, 0x280000,
    0x100000, 0x680000, 0x400000, 0x780000, 0x200000, 0x380000, 0x300000, 0x180000,
  },
  .voiceKey = 6,
  .fixBank = 1,
};

}
