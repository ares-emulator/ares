auto M68HC05::load(n16 address) -> n8 {
  step(1);
  return read(address);
}

auto M68HC05::store(n16 address, n8 data) -> void {
  step(1);
  write(address, data);
}

template<> auto M68HC05::fetch<n8>() -> n8 {
  step(1);
  return load(PC++);
}

template<> auto M68HC05::fetch<n16>() -> n16 {
  n8 lo = load(PC++);
  n8 hi = load(PC++);
  return lo << 0 | hi << 8;
}

template<> auto M68HC05::push<n8>(n8 data) -> void {
  store(0x00c0 | SP--, data);
}

template<> auto M68HC05::push<n16>(n16 data) -> void {
  store(0x00c0 | SP--, data >> 8);
  store(0x00c0 | SP--, data >> 0);
}

template<> auto M68HC05::pop<n8>() -> n8 {
  return load(0x00c0 | ++SP);
}

template<> auto M68HC05::pop<n16>() -> n16 {
  n8 lo = load(0x00c0 | ++SP);
  n8 hi = load(0x00c0 | ++SP);
  return lo << 0 | hi << 8;
}
