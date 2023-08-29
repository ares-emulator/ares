
struct Hap2000 : Interface {
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
    if(address <= 0x3fff) // static
		return rom.read((address & 0x3fff));
		
    if(address < 0xbfff) // banked
		return rom.read( (romBank * 0x8000) + (address));
	
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
	  if ( address == 0x2000){
		  print(data);
		  print("\n");
		  romBank = data;
	  }
	  /*
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
	}*/
  }

  auto power() -> void override {
    romBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
  }

  n8 romBank;
};



