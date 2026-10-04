struct ParkerBros03E0 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n3 chunks[3];

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    select(address);
    if(address.bit(12)) {
      u32 segment = address.bit(10, 11);
      u32 chunk = 7;
      if(segment < 3) chunk = chunks[segment];
      return rom.read(chunk * 0x400 + (address & 0x03ff));
    }
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    select(address);
    return data;
  }

  auto power(bool reset) -> void override {
    chunks[0] = 4;
    chunks[1] = 5;
    chunks[2] = 6;
  }

  auto serialize(serializer& s) -> void override {
    s(chunks[0]);
    s(chunks[1]);
    s(chunks[2]);
  }

private:
  auto select(n16 address) -> void {
    if(address < 0x0380 || address > 0x03ff) return;
    auto chunk = address & 7;
    if(!address.bit(4)) chunks[0] = chunk;
    if(!address.bit(5)) chunks[1] = chunk;
    if(!address.bit(6)) chunks[2] = chunk;
  }
};
