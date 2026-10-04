struct VRC1 : Interface {
  using Interface::Interface;

  auto addressPRG(n32 address) -> maybe<n32> override {
    if(address < 0x8000) return {};
    return mapPRG(programBank[address >> 13 & 3], address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    auto index = address >> 12 & 7;
    if(index == 0 || index == 2 || index == 4) selectPRG8(programBank, index >> 1, data);
    if(index == 6) selectCHR(characterBank, 0, 4, data * 4);
    if(index == 7) selectCHR(characterBank, 4, 4, data * 4);
  }

  auto addressCHR(n32 address) -> maybe<n32> override {
    return mapCHR(characterBank[address >> 10 & 7], address);
  }

  auto power() -> void override {
    initializePRG(programBank);
    initializeCHR(characterBank);
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
  }

  n8 programBank[4];
  n8 characterBank[8];
};
