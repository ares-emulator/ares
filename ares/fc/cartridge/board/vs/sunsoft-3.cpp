struct Sunsoft3 : Interface {
  using Interface::Interface;

  auto addressPRG(n32 address) -> maybe<n32> override {
    if(address < 0x8000) return {};
    return mapPRG(programBank[address >> 13 & 3], address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    switch(address & 0x7800) {
    case 0x0800:
    case 0x1800:
    case 0x2800:
    case 0x3800:
      selectCHR(characterBank, address >> 11 & 6, 2, data * 2);
      break;
    case 0x7800:
      selectPRG16(programBank, 0, data);
      break;
    }
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
