struct xin1 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {}

  auto unload() -> void override {}

  auto read(n16 address) -> n8 override {
	if (address >= 0x7fc0){
		bank = (address - 0x7fc0) % bankcount;
	}
    return rom.read( address + (bank*0x8000) );
  }

  auto write(n16 address, n8 data) -> void override {
	  return;
  }

  auto power() -> void override {
	bankcount = rom.size() / 0x8000;
    bank = bankcount-1;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

  n16 bank;
  n32 bankcount;
};
