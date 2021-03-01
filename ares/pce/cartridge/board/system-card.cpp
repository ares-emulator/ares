struct SystemCard : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n8 bank, n13 address, n8 data) -> n8 override {
    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(bank << 13 | address);
    }

    return data;
  }

  auto write(n8 bank, n13 address, n8 data) -> void override {
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
  }
};
