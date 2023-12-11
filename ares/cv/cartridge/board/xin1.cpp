struct xin1 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    bankCount = rom.size() / 0x8000;
  }

  auto save() -> void override {}

  auto unload() -> void override {}

  auto read(n16 address) -> n8 override {
    if(address >= 0x7fc0) return bank = (address - 0x7fc0) % bankCount;
    return rom.read(address + (bank * 0x8000));
  }

  auto write(n16 address, n8 data) -> void override {
    return;
  }

  auto power() -> void override {
    bank = bankCount - 1;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(bankCount);
  }

  n16 bank;
  n16 bankCount;
};
