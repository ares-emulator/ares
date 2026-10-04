struct Standard : Interface {
  using Interface::Interface;

  auto addressPRG(n32 address) -> maybe<n32> override {
    if(address < 0x8000) return {};
    n32 slot = (address - 0x8000) >> 13;
    n32 bank = slot;
    if(programBanks == 5) {
      bank = programBank;
      if(slot) bank = slot + 1;
    }
    return mapPRG(bank, address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address == 0x4016) programBank = characterBank = data.bit(2);
  }

  auto addressCHR(n32 address) -> maybe<n32> override {
    return mapCHR(characterBank * 8 + (address >> 10 & 7), address);
  }

  auto power() -> void override {
    programBank = programBanks == 5;
    characterBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
  }

  n1 programBank;
  n1 characterBank;
};
