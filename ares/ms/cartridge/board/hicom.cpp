struct Hicom: Interface {
  using Interface::Interface;
  Memory::Readable < n8 > rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {}

  auto unload() -> void override {}

  auto read(n16 address, n8 data) -> n8 override {
    if (address <= 0x7fff)
      return rom.read((m_rom_bank_base * 0x8000) | (address & 0x7fff));

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if (address == 0xffff) {
      m_rom_bank_base = data % (m_rom_page_count << 1);
    }
  }

  auto power() -> void override {
    m_rom_bank_base = 0;
  }

  auto serialize(serializer & s) -> void override {
    s(m_rom_bank_base);
  }

  n8 m_rom_bank_base;
  n8 m_rom_page_count = 5;
};
