//NEO-SMA: P-ROM decryption and bankswitching, random number generator and 256KB of data.

#include "sma-tables.hpp"

static auto smaSwap(u32 value, const u8* bits, u32 count) -> u32 {
  u32 result = 0;
  for(u32 n : range(count)) result |= (value >> bits[n] & 1) << (count - 1 - n);
  return result;
}

template<typename Base>
struct Sma : Base {
  const SmaConfig& config;

  Sma(Cartridge& cartridge, u8 key, const SmaConfig& config) : Base(cartridge, key), config(config) {}

  auto dataWord(u32 address) -> n16 {
    if(address >= Base::prom.size() * 2) return 0xffff;
    n16 data = Base::prom[address >> 1];
    if(address < 0x100000) return data;
    return smaSwap(data, config.dataSwap, 16);
  }

  auto bankWord(u32 address) -> n16 {
    if(address >= 0x100000 && address < 0x100000 + config.bankSwapLength) {
      u32 mask = (1 << config.bankSwapBits) - 1;
      u32 word = (address - 0x100000) >> 1;
      word = (word & ~mask) | smaSwap(word & mask, config.bankSwap, config.bankSwapBits);
      address = 0x100000 + (word << 1);
    }
    return dataWord(address);
  }

  auto fixedWord(u32 address) -> n16 {
    u32 word = smaSwap(address >> 1, config.fixedSwap, 19);
    u32 source = config.fixedSource + (word << 1);
    return config.fixedAfterBank ? bankWord(source) : dataWord(source);
  }

  auto random() -> n16 {
    n16 value = rng;
    n1 bit = rng.bit(2) ^ rng.bit(3) ^ rng.bit(5) ^ rng.bit(6) ^ rng.bit(7) ^ rng.bit(11) ^ rng.bit(12) ^ rng.bit(15);
    rng = rng << 1 | bit;
    return value;
  }

  auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    address &= ~1;
    if(address <= 0x0bffff) return fixedWord(address);
    if(address <= 0x0fffff) return Base::prom[address >> 1];  //SMA internal ROM
    if(address >= 0x200000 && address <= 0x2fffff) {
      if(address == 0x2fe446) return 0x9a37;
      if(address == config.randomPort[0] || address == config.randomPort[1]) return random();
      return bankWord(bankBase + n20(address));
    }
    return data;
  }

  auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if((address & ~1) == config.bankRegister) {
      bankBase = 0x100000 + config.bankOffset[smaSwap(data, config.bankSelect, 6)];
    }
  }

  auto fixBankType() -> u32 override { return Base::srom.size() > 0x20000 ? config.fixBankType : 0; }

  auto power() -> void override {
    Base::power();
    bankBase = 0x100000;
    rng = 0x2345;
  }

  auto serialize(serializer& s) -> void override {
    Base::serialize(s);
    s(bankBase);
    s(rng);
  }

  u32 bankBase;
  n16 rng;
};
