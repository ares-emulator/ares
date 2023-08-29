
struct Pak4 : Interface {
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
    if(address <= 0xbfff)
		return rom.read((romBank[address>>14]<<14) + (address & 0x3fff));
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
	switch (address){
		case 0x3ffe:
			reg[0] = data;
			romBank[0] = data % bank_count;
			romBank[2] = ((reg[0] & 0x30) + reg[2]) % bank_count;
			break;
		case 0x7fff:
			reg[1] = data;
			romBank[1] = data % bank_count;
			break;
		case 0xbfff:
			reg[2] = data;
			romBank[2] = ((reg[0] & 0x30) + reg[2]) % bank_count;
			break;
	}
  }

  auto power() -> void override {
    romBank[0] = 0;
    romBank[1] = 1;
    romBank[2] = 2;
    
    reg[0] = 0;
    reg[1] = 0;
    reg[2] = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
  }

  n8 reg[4];
  n8 romBank[4];
  n8 bank_count = 0xff; // idk what values correct.
};



