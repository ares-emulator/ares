struct Namco108 : Interface {
  using Interface::Interface;

  auto addressPRG(n32 address) -> maybe<n32> override {
    if(address < 0x8000) return {};
    return mapPRG(programBank[address >> 13 & 3], address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    switch(address & 0x6001) {
    case 0x0000:
      bankSelect = data.bit(0, 2);
      break;
    case 0x0001:
      if(bankSelect <= 1) selectCHR(characterBank, bankSelect * 2, 2, data);
      if(bankSelect >= 2 && bankSelect <= 5) selectCHR(characterBank, bankSelect + 2, 1, data);
      if(bankSelect >= 6) selectPRG8(programBank, bankSelect - 6, data);
      break;
    }
  }

  auto addressCHR(n32 address) -> maybe<n32> override {
    return mapCHR(characterBank[address >> 10 & 7], address);
  }

  auto power() -> void override {
    initializePRG(programBank);
    initializeCHR(characterBank);
    bankSelect = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
    s(bankSelect);
  }

  n8 programBank[4];
  n8 characterBank[8];
  n3 bankSelect;
};
