struct ArcadeRom : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address) -> maybe<n8> override {
    if(address >= 0x0000 && (address <= 0xbfff || address < rom.size())) {
      return rom.read(address - 0x0000);
    }

    return nothing;
  }
   
  auto write(n16 address, n8 data) -> bool override {
    if(address >= 0x0000 && (address <= 0xbfff || address < rom.size())) {
      return rom.write(address - 0x0000, data), true;
    }

    return false;
  }

  auto power() -> void override {

  }

  auto serialize(serializer& s) -> void override {

  }
};
