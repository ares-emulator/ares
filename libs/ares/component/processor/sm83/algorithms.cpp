auto SM83::ADD(n8 target, n8 source, bool carry) -> n8 {
  n16 x = target + source + carry;
  n16 y = (n4)target + (n4)source + carry;
  CF = x > 0xff;
  HF = y > 0x0f;
  NF = 0;
  ZF = (n8)x == 0;
  return x;
}

auto SM83::AND(n8 target, n8 source) -> n8 {
  target &= source;
  CF = 0;
  HF = 1;
  NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::BIT(n3 index, n8 target) -> void {
  HF = 1;
  NF = 0;
  ZF = target.bit(index) == 0;
}

auto SM83::CP(n8 target, n8 source) -> void {
  n16 x = target - source;
  n16 y = (n4)target - (n4)source;
  CF = x > 0xff;
  HF = y > 0x0f;
  NF = 1;
  ZF = (n8)x == 0;
}

auto SM83::DEC(n8 target) -> n8 {
  target--;
  HF = (n4)target == 0x0f;
  NF = 1;
  ZF = target == 0;
  return target;
}

auto SM83::INC(n8 target) -> n8 {
  target++;
  HF = (n4)target == 0x00;
  NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::OR(n8 target, n8 source) -> n8 {
  target |= source;
  CF = HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::RL(n8 target) -> n8 {
  bool carry = target.bit(7);
  target = target << 1 | CF;
  CF = carry;
  HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::RLC(n8 target) -> n8 {
  target = target << 1 | target >> 7;
  CF = target.bit(0);
  HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::RR(n8 target) -> n8 {
  bool carry = target.bit(0);
  target = CF << 7 | target >> 1;
  CF = carry;
  HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::RRC(n8 target) -> n8 {
  target = target << 7 | target >> 1;
  CF = target.bit(7);
  HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::SLA(n8 target) -> n8 {
  bool carry = target.bit(7);
  target <<= 1;
  CF = carry;
  HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::SRA(n8 target) -> n8 {
  bool carry = target.bit(0);
  target = (i8)target >> 1;
  CF = carry;
  HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::SRL(n8 target) -> n8 {
  bool carry = target.bit(0);
  target >>= 1;
  CF = carry;
  HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::SUB(n8 target, n8 source, bool carry) -> n8 {
  n16 x = target - source - carry;
  n16 y = (n4)target - (n4)source - carry;
  CF = x > 0xff;
  HF = y > 0x0f;
  NF = 1;
  ZF = (n8)x == 0;
  return x;
}

auto SM83::SWAP(n8 target) -> n8 {
  target = target << 4 | target >> 4;
  CF = HF = NF = 0;
  ZF = target == 0;
  return target;
}

auto SM83::XOR(n8 target, n8 source) -> n8 {
  target ^= source;
  CF = HF = NF = 0;
  ZF = target == 0;
  return target;
}
