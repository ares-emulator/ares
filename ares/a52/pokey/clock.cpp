auto POKEY::Clock::power() -> void {
  reset();
}

auto POKEY::Clock::reset() -> void {
  prescaler64 = 19;
  prescaler15 = 78;
  polynomial4 = 0x0f;
  polynomial5 = 0x1f;
  polynomial9 = 0x1ff;
  polynomial17 = 0x1ffff;
  polynomial4History = 0;
  polynomial5History = 15;
  polynomial9History = 0;
  polynomial17History = 0;
}

auto POKEY::Clock::clock(bool enabled) -> Pulses {
  if(!enabled) {
    reset();
    return {};
  }

  advance();

  Pulses pulses = {};
  if(!--prescaler64) {
    prescaler64 = 28;
    pulses.clock64 = true;
  }
  if(!--prescaler15) {
    prescaler15 = 114;
    pulses.clock15 = true;
  }
  return pulses;
}

auto POKEY::Clock::random(bool usePolynomial9) const -> u8 {
  return usePolynomial9 ? polynomial9 >> 1 : polynomial17 >> 9;
}

auto POKEY::Clock::sample4(u32 phase) const -> bool {
  return polynomial4History >> phase & 1;
}

auto POKEY::Clock::sample5(u32 phase) const -> bool {
  return polynomial5History >> phase & 1;
}

auto POKEY::Clock::sample9(u32 phase) const -> bool {
  return polynomial9History >> phase & 1;
}

auto POKEY::Clock::sample17(u32 phase) const -> bool {
  return polynomial17History >> phase & 1;
}

auto POKEY::Clock::advance() -> void {
  auto shift = [](u32 value, u32 bits, u32 tap) -> u32 {
    auto feedback = (value ^ (value >> tap)) & 1;
    return value >> 1 | feedback << (bits - 1);
  };

  polynomial4 = shift(polynomial4, 4, 1);
  polynomial5 = shift(polynomial5, 5, 2);
  polynomial9 = shift(polynomial9, 9, 5);
  polynomial17 = shift(polynomial17, 17, 5);
  polynomial4History = polynomial4History << 1 | !(polynomial4 & 1);
  polynomial5History = polynomial5History << 1 | (polynomial5 & 1);
  polynomial9History = polynomial9History << 1 | !(polynomial9 & 1);
  polynomial17History = polynomial17History << 1 | !(polynomial17 & 1);
}
