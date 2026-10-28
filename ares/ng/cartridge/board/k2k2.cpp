//KOF2002: P-ROM 512KB block swapping with CMC50 C/S/M decryption and NEO-PCM2 V-ROM decryption.

#include "k2k2-tables.hpp"

struct K2k2 : Cmc50 {
  const K2k2Config& config;

  K2k2(Cartridge& cartridge, u8 key, const K2k2Config& config) : Cmc50(cartridge, key), config(config) {}

  auto programWord(u32 address) -> n16 override {
    u32 offset = address - config.programBase;
    if(address >= config.programBase && offset < config.programBlocks * 0x80000) {
      address = config.programBase + config.programOrder[offset >> 19] + (offset & 0x7ffff);
    }
    return prom[address >> 1];
  }

  auto readVA(n32 address) -> n8 override {
    return pcm2SwapRead(vromA, address, config.voiceKey);
  }

  auto fixBankType() -> u32 override { return srom.size() > 0x20000 ? config.fixBank : 0; }

};
