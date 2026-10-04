struct MMC1 : Interface {
  using Interface::Interface;

  auto addressPRG(n32 address) -> maybe<n32> override {
    if(address < 0x8000) return {};
    return mapPRG(programBank[address >> 13 & 3], address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    if(data.bit(7)) {
      shiftCount = 0;
      program16K = 1;
      switchLow = 1;
      return;
    }

    shiftRegister = shiftRegister >> 1 | data.bit(0) << 4;
    shiftCount = (shiftCount + 1) % 5;
    if(shiftCount) return;

    switch(address >> 13 & 3) {
    case 0:
      character4K = shiftRegister.bit(4);
      program16K = shiftRegister.bit(3);
      switchLow = shiftRegister.bit(2);
      break;
    case 1:
      if(character4K) selectCHR(characterBank, 0, 4, shiftRegister * 4);
      else selectCHR(characterBank, 0, 8, (shiftRegister & ~1) * 4);
      break;
    case 2:
      if(character4K) selectCHR(characterBank, 4, 4, shiftRegister * 4);
      break;
    case 3:
      if(program16K) selectPRG16(programBank, !switchLow, shiftRegister);
      else selectPRG32(programBank, shiftRegister >> 1);
      break;
    }
  }

  auto addressCHR(n32 address) -> maybe<n32> override {
    return mapCHR(characterBank[address >> 10 & 7], address);
  }

  auto power() -> void override {
    initializePRG(programBank);
    initializeCHR(characterBank);
    shiftRegister = 0;
    shiftCount = 0;
    program16K = 0;
    switchLow = 0;
    character4K = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
    s(shiftRegister);
    s(shiftCount);
    s(program16K);
    s(switchLow);
    s(character4K);
  }

  n8 programBank[4];
  n8 characterBank[8];
  n5 shiftRegister;
  n3 shiftCount;
  n1 program16K;
  n1 switchLow;
  n1 character4K;
};
