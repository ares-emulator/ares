#include "cmc-tables.hpp"

struct CmcContext {
  const u8* type0High;
  const u8* type0Low;
  const u8* type1High;
  const u8* type1Low;
  const u8* address8To15Xor1;
  const u8* address8To15Xor2;
  const u8* address16To23Xor1;
  const u8* address16To23Xor2;
  const u8* address0To7Xor;
};

static auto decryptCmcGraphicsInternal(u8* r0, u8* r1, u8 c0, u8 c1, const u8* table0High, const u8* table0Low,
  const u8* table1, CmcContext& context, int base, int invert) -> void {
  u8 tmp = table1[(base & 0xff) ^ context.address0To7Xor[(base >> 8) & 0xff]];
  u8 xor0 = (table0High[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01);
  u8 xor1 = (tmp & 0xfe) | (table0Low[(base >> 8) & 0xff] & 0x01);

  if(invert) {
    *r0 = c1 ^ xor0;
    *r1 = c0 ^ xor1;
    return;
  }

  *r0 = c0 ^ xor0;
  *r1 = c1 ^ xor1;
}

static auto reorderCmcMusic(n16 source, const u8 (&bits)[16]) -> n16 {
  n16 result = 0;
  for(u32 index : range(16)) result.bit(15 - index) = source.bit(bits[index]);
  return result;
}

static auto decryptCmcMusicDescramble(u32 address, u16 key) -> u32 {
  static constexpr u8 positions[8][16] = {
    { 4, 5, 9, 6,13,11,12, 0, 8, 3, 2, 1, 7,10,14,15},
    {12, 6, 0,10,14, 4,13, 5, 3, 2, 9,15,11, 8, 1, 7},
    { 9,12,11,13, 5, 2, 0, 4, 1,15, 7,10, 3,14, 6, 8},
    { 5,14,12, 1,10, 0, 6,13, 7,11, 4, 3, 9,15, 8, 2},
    {11, 0, 2, 5,12, 7, 4, 9,10, 8, 3,14,15, 6,13, 1},
    {13,10, 5, 8, 1,12,14, 6, 2, 9, 0, 7, 4, 3,15,11},
    { 2, 4, 7,12, 0, 3, 9,11,14, 1,15, 6, 8,13, 5,10},
    { 6, 1,13,15, 8, 5,10,14,11, 4,12, 2, 0, 7, 3, 9},
  };

  int block = (address >> 16) & 7;
  int aux = address & 0xffff;

  aux ^= reorderCmcMusic(key, {12,0,2,4,8,15,7,13,10,1,3,6,11,9,14,5});
  aux = reorderCmcMusic(aux, positions[block]);

  aux ^= CmcTables::Music::address0To7Xor[(aux >> 8) & 0xff];
  aux ^= CmcTables::Music::address8To15Xor[aux & 0xff] << 8;
  aux = reorderCmcMusic(aux, {7,15,14,6,5,13,12,4,11,3,10,2,9,1,8,0});

  return (block << 16) | aux;
}

struct Cmc42 : Rom {
  u8 cmcKey;
  CmcContext context;

  Cmc42(Cartridge& cartridge, u8 key) : Rom(cartridge), cmcKey(key) {
    context.type0High            = CmcTables::Cmc42::type0High;
    context.type0Low             = CmcTables::Cmc42::type0Low;
    context.type1High            = CmcTables::Cmc42::type1High;
    context.type1Low             = CmcTables::Cmc42::type1Low;
    context.address8To15Xor1     = CmcTables::Cmc42::address8To15Xor1;
    context.address8To15Xor2     = CmcTables::Cmc42::address8To15Xor2;
    context.address16To23Xor1    = CmcTables::Cmc42::address16To23Xor1;
    context.address16To23Xor2    = CmcTables::Cmc42::address16To23Xor2;
    context.address0To7Xor       = CmcTables::Cmc42::address0To7Xor;
  }

  auto decryptCromByte(n32 address) -> n8 {
    u32 wordAddress = address / 4;
    u32 byteIndex = address & 3;

    int sourceAddress = wordAddress;
    sourceAddress ^= cmcKey;
    sourceAddress ^= context.address8To15Xor1[(sourceAddress >> 16) & 0xff] << 8;
    sourceAddress ^= context.address8To15Xor2[sourceAddress & 0xff] << 8;
    sourceAddress ^= context.address16To23Xor1[sourceAddress & 0xff] << 16;
    sourceAddress ^= context.address16To23Xor2[(sourceAddress >> 8) & 0xff] << 16;
    sourceAddress ^= context.address0To7Xor[(sourceAddress >> 8) & 0xff];

    u32 cromSize = crom.size();
    if(cromSize == 0x3000000) {
      if(wordAddress < 0x2000000 / 4) sourceAddress &= (0x2000000 / 4) - 1;
      else sourceAddress = 0x2000000 / 4 + (sourceAddress & ((0x1000000 / 4) - 1));
    } else if(cromSize == 0x6000000) {
      if(wordAddress < 0x4000000 / 4) sourceAddress &= (0x4000000 / 4) - 1;
      else sourceAddress = 0x4000000 / 4 + (sourceAddress & ((0x1000000 / 4) - 1));
    } else {
      sourceAddress &= (cromSize / 4) - 1;
    }

    u8 source0 = crom[sourceAddress * 4 + 0];
    u8 source1 = crom[sourceAddress * 4 + 1];
    u8 source2 = crom[sourceAddress * 4 + 2];
    u8 source3 = crom[sourceAddress * 4 + 3];

    u8 decoded[4];
    decryptCmcGraphicsInternal(&decoded[0], &decoded[3], source0, source3,
      context.type0High, context.type0Low, context.type1High, context, sourceAddress, (sourceAddress >> 8) & 1);
    decryptCmcGraphicsInternal(&decoded[1], &decoded[2], source1, source2,
      context.type0Low, context.type0High, context.type1Low, context, sourceAddress,
      ((sourceAddress >> 16) ^ context.address16To23Xor2[(sourceAddress >> 8) & 0xff]) & 1);

    return decoded[byteIndex];
  }

  auto readC(n32 address) -> n8 override {
    return decryptCromByte(address);
  }

  auto readS(n32 address) -> n8 override {
    u32 cromSize = crom.size();
    u32 sromSize = srom.size();
    u32 i = address;
    u32 cromAddr = (cromSize - sromSize) + ((i & ~0x1f) + ((i & 7) << 2) + ((~i & 8) >> 2) + ((i & 0x10) >> 4));
    return decryptCromByte(cromAddr);
  }
};

struct Cmc50 : Cmc42 {
  u16 musicKey;
  u32 fixBank;

  Cmc50(Cartridge& cartridge, u8 key, u32 fixBank = 0) : Cmc42(cartridge, key), fixBank(fixBank) {
    context.type0High            = CmcTables::Cmc50::type0High;
    context.type0Low             = CmcTables::Cmc50::type0Low;
    context.type1High            = CmcTables::Cmc50::type1High;
    context.type1Low             = CmcTables::Cmc50::type1Low;
    context.address8To15Xor1     = CmcTables::Cmc50::address8To15Xor1;
    context.address8To15Xor2     = CmcTables::Cmc50::address8To15Xor2;
    context.address16To23Xor1    = CmcTables::Cmc50::address16To23Xor1;
    context.address16To23Xor2    = CmcTables::Cmc50::address16To23Xor2;
    context.address0To7Xor       = CmcTables::Cmc50::address0To7Xor;
  }

  auto load() -> void override {
    Rom::load();
    musicKey = 0;
    for(u32 i = 0; i < 0x10000; i++) musicKey += mrom[i];
  }

  auto fixBankType() -> u32 override { return srom.size() > 0x20000 ? fixBank : 0; }

  auto readM(n32 address) -> n8 override {
    return mrom[decryptCmcMusicDescramble(address, musicKey)];
  }
};
