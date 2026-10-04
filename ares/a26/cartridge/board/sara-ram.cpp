struct SaraRam {
  SaraRam(bool present = false) : present(present) {}

  auto readable(n16 address) -> bool {
    return present && address >= 0x1000 && address <= 0x10ff;
  }

  auto read(n16 address, n8 data) -> n8 {
    if(address <= 0x107f) {
      ram.write(address & 0x7f, data);
      return data;
    }
    return ram.read(address & 0x7f);
  }

  auto write(n16 address, n8 data) -> bool {
    if(!present || address < 0x1000 || address > 0x107f) return false;
    ram.write(address & 0x7f, data);
    return true;
  }

  auto power() -> void {
    if(present) ram.allocate(128, 0x00);
  }

  auto serialize(serializer& s) -> void {
    if(present) s(ram);
  }

  const bool present;
  Memory::Writable<n8> ram;
};
