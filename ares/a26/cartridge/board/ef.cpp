struct EFBase : Interface {
  EFBase(Cartridge& cartridge, n16 hotspotMask, n16 hotspot, n6 startupBank, bool hasRam = false)
  : Interface(cartridge), hotspotMask(hotspotMask), hotspot(hotspot), startupBank(startupBank), hasRam(hasRam) {}

  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  const n16 hotspotMask;
  const n16 hotspot;
  const n6 startupBank;
  const bool hasRam;
  n6 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    bankswitch(address);
    if(!address.bit(12)) return data;
    if(hasRam && address <= 0x107f) {
      ram.write(address & 0x7f, data);
      return data;
    }
    if(hasRam && address <= 0x10ff) return ram.read(address & 0x7f);
    return rom.read(bank * 0x1000 + (address & 0x0fff));
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    bankswitch(address);
    if(hasRam && address >= 0x1000 && address <= 0x107f) {
      ram.write(address & 0x7f, data);
      return data;
    }
    return data;
  }

  auto power(bool reset) -> void override {
    bank = startupBank;
    if(hasRam) ram.allocate(128, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    if(hasRam) s(ram);
  }

private:
  auto bankswitch(n16 address) -> void {
    if((address & hotspotMask) == hotspot) bank = address - hotspot;
  }
};

struct EF : EFBase {
  EF(Cartridge& cartridge) : EFBase(cartridge, 0x1ff0, 0x1fe0, 1) {}
};

struct DF : EFBase {
  DF(Cartridge& cartridge) : EFBase(cartridge, 0x1fe0, 0x1fc0, 15) {}
};

struct BF : EFBase {
  BF(Cartridge& cartridge) : EFBase(cartridge, 0x1fc0, 0x1f80, 1) {}
};

struct EFSC : EFBase {
  EFSC(Cartridge& cartridge) : EFBase(cartridge, 0x1ff0, 0x1fe0, 15, true) {}
};

struct DFSC : EFBase {
  DFSC(Cartridge& cartridge) : EFBase(cartridge, 0x1fe0, 0x1fc0, 15, true) {}
};

struct BFSC : EFBase {
  BFSC(Cartridge& cartridge) : EFBase(cartridge, 0x1fc0, 0x1f80, 15, true) {}
};
