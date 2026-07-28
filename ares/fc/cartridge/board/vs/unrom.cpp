struct UNROM : Interface {
  using Interface::Interface;

  auto addressPRG(n32 address) -> maybe<n32> override {
    if(address < 0x8000) return {};
    return mapPRG(programBank[address >> 13 & 3], address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x8000) selectPRG16(programBank, 0, data);
  }

  auto addressCHR(n32 address) -> maybe<n32> override {
    return mapCHR(address >> 10 & 7, address);
  }

  auto power() -> void override {
    initializePRG(programBank);
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
  }

  n8 programBank[4];
};
