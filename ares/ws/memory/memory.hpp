struct IO {
  virtual auto readIO(n16 address) -> n8 = 0;
  virtual auto writeIO(n16 address, n8 data) -> void = 0;
};

struct InternalRAM {
  auto power() -> void;
  auto serialize(serializer&) -> void;

  auto read(n16 address) -> n8 {
    if(address >= size) return 0x90;
    return memory[address & maskByte];
  }

  auto write(n16 address, n8 data) -> void {
    if(address >= size) return;
    memory[address & maskByte] = data;
  }

  auto read8(n16 address) const -> n16 {
    return memory[address & maskByte];
  }

  auto read16(n16 address) const -> n16 {
    n8 d0 = memory[address & maskWord | 0];
    n8 d1 = memory[address & maskWord | 1];
    return d0 << 0 | d1 << 8;
  }

  auto read32(n16 address) const -> n32 {
    n8 d0 = memory[address & maskLong | 0];
    n8 d1 = memory[address & maskLong | 1];
    n8 d2 = memory[address & maskLong | 2];
    n8 d3 = memory[address & maskLong | 3];
    return d0 << 0 | d1 << 8 | d2 << 16 | d3 << 24;
  }

private:
  n8  memory[65536];
  n32 size;
  n32 maskByte;
  n32 maskWord;
  n32 maskLong;
};

struct Bus {
  auto power() -> void;

  auto read(n20 address) -> n8;
  auto write(n20 address, n8 data) -> void;

  auto map(IO* io, u16 lo, maybe<u16> hi = nothing) -> void;
  auto readIO(n16 address) -> n8;
  auto writeIO(n16 address, n8 data) -> void;

private:
  IO* port[64 * 1024] = {nullptr};
};

extern InternalRAM iram;
extern Bus bus;
