//Cross Blaim

struct CrossBlaim : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(mode == 0 || mode == 1) {
      if(address >> 14 == 0) data = rom.read((n14)address + 16_KiB);
      if(address >> 14 == 1) data = rom.read((n14)address);
      if(address >> 14 == 2) data = rom.read((n14)address + 16_KiB);
      if(address >> 14 == 3) data = rom.read((n14)address + 16_KiB);
    }

    if(mode == 2) {
      if(address >> 14 == 0) data = 0xff;
      if(address >> 14 == 1) data = rom.read((n14)address);
      if(address >> 14 == 2) data = rom.read((n14)address + 32_KiB);
      if(address >> 14 == 3) data = 0xff;
    }

    if(mode == 3) {
      if(address >> 14 == 0) data = 0xff;
      if(address >> 14 == 1) data = rom.read((n14)address);
      if(address >> 14 == 2) data = rom.read((n14)address + 48_KiB);
      if(address >> 14 == 3) data = 0xff;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    mode = data;
  }

  auto power() -> void override {
    mode = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(mode);
  }

  n2 mode;
};
