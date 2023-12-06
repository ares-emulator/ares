struct megacart : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {}

  auto unload() -> void override {}

  auto read(n16 address) -> n8 override {
	if (address >= 0x7FC0){
	  bank = address & (bankcount - 1);
	}
	if (address >= 0x4000){
		return rom.read( (bank << 14) + (address - 0x4000) );
	}
    return rom.read( (bankcount << 14) + (address - 0x4000) );
  }

  auto write(n16 address, n8 data) -> void override {
	  return;
  }

  auto power() -> void override {
	bankcount = rom.size() >> 14;
    bank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }


  n16 bank;
  n16 bankcount;
};
