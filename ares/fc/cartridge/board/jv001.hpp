struct JV001 {
  auto power() -> void {
    in = 0;
    out = 0;
    reg = 0;
    mode = 0;
    invert = 1;
  }

  auto write(n16 address, n8 data) -> void {
    if (address >= 0x4100 && address < 0x4104) {
      switch (address & 0x03) {
        case 0: accumulator = mode ? (accumulator + 1) : (staging ^ (invert ? 0x0f : 0));
                if (!mode) regInverter = inverter;
                break;
        case 1: invert = data.bit(0); break;
        case 2: in = data; break;
        case 3: mode = data.bit(0); break;
      }
    }

    if (address >= 0x8000)
      out = reg;
  }

  auto read() -> n8 {
    return (regInverter ^ (invert ? 0x03 : 0)) << 4 | accumulator;
  }

  auto serialize(serializer& s) -> void {
    s(in);
    s(out);
    s(reg);
    s(mode);
    s(invert);
  }

  n6 in;
  n6 out;
  n6 reg;
  n1 mode;    // increase
  n1 invert;

  BitRange<6, 0, 3> staging{&in};
  BitRange<6, 4, 5> inverter{&in};
  BitRange<6, 0, 3> accumulator{&reg};
  BitRange<6, 4, 5> regInverter{&reg};
};
