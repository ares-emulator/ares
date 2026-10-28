// NEO-MVS PROGSF1
struct Kof98 : Rom {
  using Rom::Rom;

  auto rawWord(u32 address) -> n16 {
    if(address >= prom.size() * 2) return 0xffff;
    return prom[address >> 1];
  }

  auto programWord(u32 address) -> n16 override {
    if(address >= 0x100000) return rawWord(address + 0x100000);
    if(address < 0x800) return rawWord(address);

    static const u32 sec[] = {0x000000, 0x100000, 0x000004, 0x100004, 0x10000a, 0x00000a, 0x10000e, 0x00000e};
    u32 block = address & ~0x1ff;
    u32 half  = address & 0x100;
    u32 group = address & 0xf0;
    u32 word  = address & 0x0e;
    u32 other = half ^ 0x100;

    if(!group && word == 0) return rawWord(block | half);
    if(!group && word == 2) return rawWord(block | half | 0x100000);
    if(block >= 0x080000 && (word == 0x0 || word == 0x4 || word == 0xa || word == 0xe)) {
      if(block < 0x0c0000) return rawWord(address);
      return rawWord(block | other | group | word);
    }
    return rawWord((block | other | group) + sec[word >> 1]);
  }

  auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(address >= 0x100 && address <= 0x103) {
      n1 second = address.bit(1);
      if(state == 1) return second ? 0x00fd : 0x00c2;
      if(state == 2) return second ? 0x4f2d : 0x4e45;
    }
    return Rom::readP(upper, lower, address, data);
  }

  auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if((address & ~1) == 0x20aaaa) {
      if(data == 0x0090) state = 1;
      if(data == 0x00f0) state = 2;
      return;
    }
    Rom::writeP(upper, lower, address, data);
  }

  auto power() -> void override {
    Rom::power();
    state = 0;
  }

  auto serialize(serializer& s) -> void override {
    Rom::serialize(s);
    s(state);
  }

  n2 state;
};
