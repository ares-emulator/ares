//NEO-PVC: P-ROM decryption and bankswitching 8KB of cartridge RAM with color pack/unpack helpers.

#include "pvc-tables.hpp"

struct PvcRegisters {
  auto allocate() -> void { ram.allocate(0x2000 >> 1); }
  auto reset() -> void { ram.reset(); }

  auto power() -> void {
    for(u32 n : range(ram.size())) ram[n] = 0;
    bankBase = 0x100000;
  }

  auto read(n24 address) -> n16 {
    return ram[n12(address >> 1)];
  }

  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void {
    n12 offset = address >> 1;
    n16 word = ram[offset];
    if(upper) word.byte(1) = data.byte(1);
    if(lower) word.byte(0) = data.byte(0);
    ram[offset] = word;

    if(offset == 0xff0) unpackColor();
    if(offset == 0xff4 || offset == 0xff5) packColor();
    if(offset >= 0xff8) {
      bankBase = 0x100000 + (ram[0xff8] >> 8 | ram[0xff9] << 8);
      ram[0xff8] = (ram[0xff8] & 0xfe00) | 0x00a0;
      ram[0xff9] = ram[0xff9] & 0x7fff;
    }
  }

  auto unpackColor() -> void {
    n16 pen = ram[0xff0];
    n8 b = (pen & 0x000f) << 1 | (pen & 0x1000) >> 12;
    n8 g = (pen & 0x00f0) >> 3 | (pen & 0x2000) >> 13;
    n8 r = (pen & 0x0f00) >> 7 | (pen & 0x4000) >> 14;
    n8 s = (pen & 0x8000) >> 15;
    ram[0xff1] = g << 8 | b;
    ram[0xff2] = s << 8 | r;
  }

  auto packColor() -> void {
    n16 gb = ram[0xff4];
    n16 sr = ram[0xff5];
    ram[0xff6] = (gb & 0x001e) >> 1 | (gb & 0x1e00) >> 5 | (sr & 0x001e) << 7
               | (gb & 0x0001) << 12 | (gb & 0x0100) << 5 | (sr & 0x0001) << 14 | (sr & 0x0100) << 7;
  }

  auto serialize(serializer& s) -> void {
    s(ram);
    s(bankBase);
  }

  Memory::Writable<n16> ram;
  u32 bankBase;
};

struct Pvc : Cmc50 {
  const PvcConfig& config;
  PvcRegisters registers;

  Pvc(Cartridge& cartridge, u8 key, const PvcConfig& config) : Cmc50(cartridge, key), config(config) {}

  auto load() -> void override {
    Cmc50::load();
    registers.allocate();
  }

  auto unload() -> void override {
    Cmc50::unload();
    registers.reset();
  }

  auto rawByte(u32 address) -> n8 {
    if(address >= prom.size() * 2) return 0xff;
    n16 word = prom[address >> 1];
    return address & 1 ? word.byte(1) : word.byte(0);
  }

  auto xorByte(u32 address) -> n8 {
    if(config.hasThirdProgramRom && address >= 0x800000) return rawByte(address) ^ rawByte(0x100002 | (address & 0xfffff));
    if(address < 0x100000) return rawByte(address) ^ config.fixedXor[address & 31];
    return rawByte(address) ^ config.bankedXor[address & 31];
  }

  auto dataByte(u32 address) -> n8 {
    if(address < 0x100000 || address >= 0x800000 || !(address & 3) || (address & 3) == 3) return xorByte(address);
    u32 base = address & ~3;
    u16 value = xorByte(base + 1) | xorByte(base + 2) << 8;
    value = smaSwap(value, config.dataSwap, 16);
    return (address & 3) == 1 ? value & 0xff : value >> 8;
  }

  auto decryptByte(u32 address) -> n8 {
    u32 top = config.hasThirdProgramRom ? 0x800000 : 0x700000;
    if(address >= 0x100000 && address < 0x200000) address += top - 0x100000;
    else if(address >= 0x200000) address -= 0x100000;

    if(address < 0x100000) {
      u32 block = address >> 16;
      block = (block & 0xf0) | smaSwap(block & 0x0f, config.fixedBlock, 4);
      return dataByte(block << 16 | (address & 0xffff));
    }
    u32 source = (address & 0xf000ff) + ((address & 0x000f00) ^ config.chunkXor);
    source += smaSwap(address >> 12 & 0xff, config.bankBlock, 8) << 12;
    return dataByte(source);
  }

  auto decryptWord(u32 address) -> n16 {
    return decryptByte(address) | decryptByte(address + 1) << 8;
  }

  auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    address &= ~1;
    if(address <= 0x0fffff) return decryptWord(address);
    if(address >= 0x2fe000 && address <= 0x2fffff) return registers.read(address);
    if(address >= 0x200000 && address <= 0x2fffff) return decryptWord(registers.bankBase + n20(address));
    return data;
  }

  auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(address >= 0x2fe000 && address <= 0x2fffff) registers.write(upper, lower, address, data);
  }

  auto readVA(n32 address) -> n8 override {
    return pcm2SwapRead(vromA, address, config.voiceKey);
  }

  auto fixBankType() -> u32 override { return srom.size() > 0x20000 ? config.fixBank : 0; }

  auto power() -> void override {
    Cmc50::power();
    registers.power();
  }

  auto serialize(serializer& s) -> void override {
    Cmc50::serialize(s);
    registers.serialize(s);
  }
};
