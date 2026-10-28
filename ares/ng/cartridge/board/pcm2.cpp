//NEO-PCM2 (SNK 1999): V-ROM address line scramble

#include "pcm2-tables.hpp"

struct Pcm2 : Cmc50 {
  u32 voiceXor;
  u32 fixBank;

  Pcm2(Cartridge& cartridge, u8 key, u32 voiceKey, u32 fixBank) : Cmc50(cartridge, key), voiceXor(voiceKey / 2), fixBank(fixBank) {}

  auto readVA(n32 address) -> n8 override {
    return vromA[address ^ voiceXor];
  }

  auto fixBankType() -> u32 override { return srom.size() > 0x20000 ? fixBank : 0; }
};

static auto pcm2SwapRead(Memory::Readable<n8>& vrom, u32 address, u32 key) -> n8 {
  const auto& config = Pcm2Tables::keys[key];
  u32 sampleAddress = address & 0xffffff;
  u32 sourceAddress = sampleAddress ^ config.addressXor;
  sourceAddress = (sourceAddress & ~0x10001) | (sourceAddress >> 16 & 1) | (sourceAddress & 1) << 16;  //swap address lines 0 and 16
  return vrom[(sourceAddress + config.addressOffset) & 0xffffff] ^ config.dataXor[sampleAddress & 7];
}
