//PRO-CT0 (ALPHA-8921 / SNK-9201)

struct ProCT0 {
  auto clock(n1 load, n1 h, n32 c) -> void {
    if(load) {
      sr = c;
    } else if(h) {
      sr = sr.bit(24,29) << 26 | sr.bit(16,21) << 18 | sr.bit(8,13) << 10 | sr.bit(0,5) << 2;
    } else {
      sr = sr.bit(26,31) << 24 | sr.bit(18,23) << 16 | sr.bit(10,15) << 8 | sr.bit(2,7);
    }
  }

  auto output(n1 even, n1 h) const -> std::pair<n4, n4> {
    n4 gad, gbd;
    if(h) {
      gbd = sr.bit(30) << 3 | sr.bit(22) << 2 | sr.bit(14) << 1 | sr.bit(6);
      gad = sr.bit(31) << 3 | sr.bit(23) << 2 | sr.bit(15) << 1 | sr.bit(7);
    } else {
      gbd = sr.bit(25) << 3 | sr.bit(17) << 2 | sr.bit(9) << 1 | sr.bit(1);
      gad = sr.bit(24) << 3 | sr.bit(16) << 2 | sr.bit(8) << 1 | sr.bit(0);
    }
    if(even) std::swap(gad, gbd);
    return {gad, gbd};
  }

  n32 sr;
};

struct FatFury2 : Rom {
  using Rom::Rom;

  static auto interleave(n16 value) -> n16 {
    static const u8 bits[16] = {15,11,14,10,13,9,12,8,7,3,6,2,5,1,4,0};
    return smaSwap(value, bits, 16);
  }

  auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(address <= 0x0fffff) return prom[address >> 1];
    if(address >= 0x200000 && address <= 0x2fffff) {
      auto [gad, gbd] = proct0.output(address.bit(2), address.bit(3));
      return gbd.bit(0,1) << 6 | gbd.bit(2,3) << 4 | gad.bit(0,1) << 2 | gad.bit(2,3);
    }
    return data;
  }

  auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(address < 0x200000 || address > 0x2fffff) return;
    n32 c = interleave(address.bit(4,19)) << 16 | interleave(data);
    proct0.clock(address.bit(1), address.bit(3), c);
  }

  auto power() -> void override {
    Rom::power();
    proct0 = {};
  }

  auto serialize(serializer& s) -> void override {
    Rom::serialize(s);
    s(proct0.sr);
  }

  ProCT0 proct0;
};
