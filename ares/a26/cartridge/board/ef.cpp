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

struct EFF : EFBase {
  EFF(Cartridge& cartridge) : EFBase(cartridge, 0x1ff0, 0x1fe0, 1) {}

  M24C eeprom;
  PersistentMemory persistent;

  auto load() -> void override {
    EFBase::load();
    persistent.load(pak, "save.eeprom", 2_KiB, 0xff);
    eeprom.load(M24C::Type::M24C16);
    std::copy(persistent.memory.begin(), persistent.memory.end(), std::begin(eeprom.memory));
  }

  auto save() -> void override {
    persistent.flush(pak);
  }

  auto unload() -> void override {
    persistent.flush(pak);
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address >= 0x1ff0 && address <= 0x1ff4) {
      if(address == 0x1ff0) setClock(0);
      if(address == 0x1ff1) setClock(1);
      if(address == 0x1ff2) setData(0);
      if(address == 0x1ff3) setData(1);
      if(address == 0x1ff4) return rom.read(bank * 0x1000 + 0x0ff4 + eeprom.read());
    }
    return EFBase::read(address, data);
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address >= 0x1ff0 && address <= 0x1ff3) {
      if(address == 0x1ff0) setClock(0);
      if(address == 0x1ff1) setClock(1);
      if(address == 0x1ff2) setData(0);
      if(address == 0x1ff3) setData(1);
    }
    return EFBase::write(address, data);
  }

  auto power(bool reset) -> void override {
    EFBase::power(reset);
    eeprom.power();
  }

  auto serialize(serializer& s) -> void override {
    EFBase::serialize(s);
    eeprom.serialize(s, false);
    if(s.reading()) {
      std::copy(persistent.memory.begin(), persistent.memory.end(), std::begin(eeprom.memory));
    }
  }

private:
  auto synchronizePersistent() -> void {
    persistent.replace({(const u8*)eeprom.memory, eeprom.size()});
  }

  auto setClock(bool value) -> void {
    eeprom.clock = eeprom.clock();
    eeprom.data = eeprom.data();
    eeprom.clock = value;
    eeprom.write();
    synchronizePersistent();
  }

  auto setData(bool value) -> void {
    eeprom.clock = eeprom.clock();
    eeprom.data = eeprom.data();
    eeprom.data = value;
    eeprom.write();
    synchronizePersistent();
  }
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
