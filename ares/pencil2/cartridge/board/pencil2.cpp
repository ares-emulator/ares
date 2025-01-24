struct pencil2 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {}

  auto unload() -> void override {}

  auto read(n16 address) -> n8 override {
    return rom.read(address);
  }

  auto write(n16 address, n8 data) -> void override {
    return;
  }

  auto power() -> void override {}

  auto serialize(serializer& s) -> void override {}

};
