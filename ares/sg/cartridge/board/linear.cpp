struct Linear : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
  }

  auto save() -> void override {
    Interface::save(ram, "save.ram");
  }

  auto unload() -> void override {
  }

  auto read(n16 address) -> maybe<n8> override {
    if(address >= 0x0000 && (address <= 0x7fff || address < rom.size())) {
      return rom.read(address - 0x0000);
    }

    if(address >= 0x8000 && address <= 0xbfff) {
      return ram.read(address - 0x8000);
    }

    return nothing;
  }
   
  auto write(n16 address, n8 data) -> bool override {
    if(address >= 0x0000 && (address <= 0x7fff || address < rom.size())) {
      return rom.write(address - 0x0000, data), true;
    }

    if(address >= 0x8000 && address <= 0xbfff) {
      return ram.write(address - 0x8000, data), true;
    }

    return false;
  }

  auto power() -> void override {

  }

  auto serialize(serializer& s) -> void override {
    s(ram);
  }
};
