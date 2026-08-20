struct Protection {
  virtual ~Protection() = default;

  static auto create(string id) -> Protection*;

  virtual auto readPRG(n32 address, n8 data) -> n8 { return data; }
  virtual auto power() -> void {}
  virtual auto serialize(serializer& s) -> void {}
};

struct RBIBaseball : Protection {
  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address != 0x5600 && address != 0x5601 && address != 0x5e00 && address != 0x5e01) return data;
    if(!(address & 1)) {
      index = 0;
      return 0;
    }
    return sequence[index++ & 0x1f];
  }

  auto power() -> void override {
    index = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(index);
  }

  static constexpr u8 sequence[32] = {
    0xff, 0xfd, 0xf5, 0xf4, 0xb4, 0xb4, 0xa6, 0x2e,
    0x2f, 0x6f, 0x6f, 0x7d, 0xd5, 0xd4, 0x94, 0x94,
    0x86, 0x2e, 0x2f, 0x6f, 0x6b, 0x79, 0xd1, 0xd0,
    0x92, 0x92, 0x8d, 0x65, 0x64, 0x34, 0xb0, 0xa2,
  };

  n8 index;
};

struct TKOBoxing : Protection {
  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address != 0x5e00 && address != 0x5e01) return data;
    if(!(address & 1)) {
      index = 0;
      return 0;
    }
    return sequence[index++ & 0x1f];
  }

  auto power() -> void override {
    index = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(index);
  }

  static constexpr u8 sequence[32] = {
    0xff, 0xbf, 0xb7, 0x97, 0x97, 0x17, 0x57, 0x4f,
    0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90, 0x94, 0x14,
    0x56, 0x4e, 0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90,
    0xd4, 0x5c, 0x3e, 0x26, 0x87, 0x83, 0x13, 0x00,
  };

  n8 index;
};

struct SuperXevious : Protection {
  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address == 0x54ff) {
      index ^= 1;
      return 0x05;
    }
    if(address == 0x5678) return index ? 0x01 : 0x00;
    if(address == 0x578f) return index ? 0x89 : 0xd1;
    if(address == 0x5567) return index ? 0x37 : 0x3e;
    return data;
  }

  auto power() -> void override {
    index = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(index);
  }

  n1 index;
};
