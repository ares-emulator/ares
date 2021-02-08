struct Register {
  n16 data = 0;
  bool modified = false;

  operator u32() const {
    return data;
  }

  auto assign(u32 value) -> n16 {
    modified = true;
    return data = value;
  }

  auto operator++() { return assign(data + 1); }
  auto operator--() { return assign(data - 1); }
  auto operator++(s32) { u32 r = data; assign(data + 1); return r; }
  auto operator--(s32) { u32 r = data; assign(data - 1); return r; }
  auto operator   = (u32 i) { return assign(i); }
  auto operator  |= (u32 i) { return assign(data | i); }
  auto operator  ^= (u32 i) { return assign(data ^ i); }
  auto operator  &= (u32 i) { return assign(data & i); }
  auto operator <<= (u32 i) { return assign(data << i); }
  auto operator >>= (u32 i) { return assign(data >> i); }
  auto operator  += (u32 i) { return assign(data + i); }
  auto operator  -= (u32 i) { return assign(data - i); }
  auto operator  *= (u32 i) { return assign(data * i); }
  auto operator  /= (u32 i) { return assign(data / i); }
  auto operator  %= (u32 i) { return assign(data % i); }

  auto operator   = (const Register& value) { return assign(value); }

  Register() = default;
  Register(const Register&) = delete;
};

struct SFR {
  n16 data;
  BitField<16, 1> z   {&data};  //zero flag
  BitField<16, 2> cy  {&data};  //carry flag
  BitField<16, 3> s   {&data};  //sign flag
  BitField<16, 4> ov  {&data};  //overflow flag
  BitField<16, 5> g   {&data};  //go flag
  BitField<16, 6> r   {&data};  //ROM r14 flag
  BitField<16, 8> alt1{&data};  //alt1 instruction mode
  BitField<16, 9> alt2{&data};  //alt2 instruction mode
  BitField<16,10> il  {&data};  //immediate lower 8-bit flag
  BitField<16,11> ih  {&data};  //immediate upper 8-bit flag
  BitField<16,12> b   {&data};  //with flag
  BitField<16,15> irq {&data};  //interrupt flag

  BitRange<16,8,9> alt{&data};  //composite instruction mode

  SFR() = default;
  SFR(const SFR&) = delete;
  auto operator=(const SFR&) = delete;

  operator u32() const { return data & 0x9f7e; }
  auto& operator=(const u32 value) { return data = value, *this; }
};

struct SCMR {
  u32  ht;
  bool ron;
  bool ran;
  u32  md;

  operator u32() const {
    return ((ht >> 1) << 5) | (ron << 4) | (ran << 3) | ((ht & 1) << 2) | (md);
  }

  auto& operator=(u32 data) {
    ht  = (bool)(data & 0x20) << 1;
    ht |= (bool)(data & 0x04) << 0;
    ron = data & 0x10;
    ran = data & 0x08;
    md  = data & 0x03;
    return *this;
  }
};

struct POR {
  bool obj;
  bool freezehigh;
  bool highnibble;
  bool dither;
  bool transparent;

  operator u32() const {
    return (obj << 4) | (freezehigh << 3) | (highnibble << 2) | (dither << 1) | (transparent);
  }

  auto& operator=(u32 data) {
    obj         = data & 0x10;
    freezehigh  = data & 0x08;
    highnibble  = data & 0x04;
    dither      = data & 0x02;
    transparent = data & 0x01;
    return *this;
  }
};

struct CFGR {
  bool irq;
  bool ms0;

  operator u32() const {
    return (irq << 7) | (ms0 << 5);
  }

  auto& operator=(u32 data) {
    irq = data & 0x80;
    ms0 = data & 0x20;
    return *this;
  }
};

struct Registers {
  n8 pipeline;
  n16 ramaddr;

  Register r[16];  //general purpose registers
  SFR sfr;         //status flag register
  n8 pbr;          //program bank register
  n8 rombr;        //game pack ROM bank register
  bool rambr;      //game pack RAM bank register
  n16 cbr;         //cache base register
  n8 scbr;         //screen base register
  SCMR scmr;       //screen mode register
  n8 colr;         //color register
  POR por;         //plot option register
  bool bramr;      //back-up RAM register
  n8 vcr;          //version code register
  CFGR cfgr;       //config register
  bool clsr;       //clock select register

  u32 romcl;       //clock ticks until romdr is valid
  n8 romdr;        //ROM buffer data register

  u32 ramcl;       //clock ticks until ramdr is valid
  n16 ramar;       //RAM buffer address register
  n8 ramdr;        //RAM buffer data register

  u32 sreg;
  u32 dreg;
  auto& sr() { return r[sreg]; }  //source register (from)
  auto& dr() { return r[dreg]; }  //destination register (to)

  auto reset() -> void {
    sfr.b    = 0;
    sfr.alt1 = 0;
    sfr.alt2 = 0;

    sreg = 0;
    dreg = 0;
  }
} regs;

struct Cache {
  n8 buffer[512];
  bool valid[32];
} cache;

struct PixelCache {
  n16 offset;
  n8 bitpend;
  n8 data[8];
} pixelcache[2];
