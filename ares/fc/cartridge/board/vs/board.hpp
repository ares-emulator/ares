struct Interface {
  Interface(u32 programSize, u32 characterSize)
  : programBanks(programSize / 0x2000), characterBanks(characterSize / 0x0400) {}
  virtual ~Interface() = default;

  static auto create(string id, u32 programSize, u32 characterSize) -> Interface*;

  virtual auto addressPRG(n32 address) -> maybe<n32> { return {}; }
  virtual auto writePRG(n32 address, n8 data) -> void {}
  virtual auto addressCHR(n32 address) -> maybe<n32> { return {}; }
  virtual auto power() -> void {}
  virtual auto serialize(serializer& s) -> void {}

protected:
  auto mapPRG(n32 bank, n32 address) -> maybe<n32> {
    if(!programBanks) return {};
    return {(bank % programBanks) * 0x2000 | (address & 0x1fff)};
  }

  auto mapCHR(n32 bank, n32 address) -> maybe<n32> {
    if(!characterBanks) return {};
    return {(bank % characterBanks) * 0x0400 | (address & 0x03ff)};
  }

  auto initializePRG(n8 banks[4]) -> void {
    for(auto slot : range(4)) banks[slot] = 0;
    if(programBanks < 4) return;
    for(auto slot : range(4)) banks[slot] = programBanks - 4 + slot;
  }

  auto initializeCHR(n8 banks[8]) -> void {
    for(auto slot : range(8)) banks[slot] = slot;
  }

  auto selectPRG8(n8 banks[4], u32 slot, n32 bank) -> void {
    if(!programBanks) return;
    banks[slot & 3] = bank & (programBanks - 1);
  }

  auto selectPRG16(n8 banks[4], u32 slot, n32 bank) -> void {
    if(!programBanks) return;
    bank = bank * 2 & (programBanks - 1);
    slot = (slot & 1) * 2;
    banks[slot + 0] = bank + 0;
    banks[slot + 1] = bank + 1;
  }

  auto selectPRG32(n8 banks[4], n32 bank) -> void {
    if(!programBanks) return;
    bank = bank * 4 & (programBanks - 1);
    for(auto slot : range(4)) banks[slot] = bank + slot;
  }

  auto selectCHR(n8 banks[8], u32 start, u32 count, n32 bank) -> void {
    if(!characterBanks) return;
    bank &= characterBanks - 1;
    for(auto slot : range(count)) banks[start + slot] = bank + slot;
  }

  u32 programBanks;
  u32 characterBanks;
};
