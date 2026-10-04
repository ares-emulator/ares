struct Linear : Interface {
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
    if(address.bit(12)) return rom.read(address & 0xfff);

    return data;
  }
   
  auto write(n16 address, n8 data) -> n8 override {
    return data;
  }

  auto power(bool reset) -> void override {
  }

  auto serialize(serializer& s) -> void override {
  }
};
