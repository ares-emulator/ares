struct IO {
  virtual auto portRead(n16 address) -> n8 = 0;
  virtual auto portWrite(n16 address, n8 data) -> void = 0;
};

struct InternalRAM {
  auto power() -> void;
  auto serialize(serializer&) -> void;

  auto read(n16 address) -> n8;
  auto write(n16 address, n8 data) -> void;

  //PPU byte reads only:
  //WS: address is always < 0x4000
  auto read8(n16 address) const -> n16 {
    return memory[address];
  }

  //PPU word reads only:
  //address & 1 is always 0
  //WS: address is always < 0x4000
  auto read16(n16 address) const -> n16 {
    return memory[address + 0] << 0 | memory[address + 1] << 8;
  }

  //PPU long reads only:
  //address & 3 is always 0
  //WS: address is always < 0x4000
  auto read32(n16 address) const -> n32 {
    return memory[address + 0] <<  0 | memory[address + 1] <<  8
         | memory[address + 2] << 16 | memory[address + 3] << 24;
  }

private:
  n8 memory[65536];
};

struct Bus {
  auto power() -> void;

  auto read(n20 address) -> n8;
  auto write(n20 address, n8 data) -> void;

  auto map(IO* io, u16 lo, maybe<u16> hi = nothing) -> void;
  auto portRead(n16 address) -> n8;
  auto portWrite(n16 address, n8 data) -> void;

private:
  IO* port[64 * 1024] = {nullptr};
};

extern InternalRAM iram;
extern Bus bus;
